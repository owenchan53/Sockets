#include <atomic>
#include <iostream>
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <thread>
#include <chrono>
#include <cmath>

class MultiServer
{
private:
    std::atomic<bool> stopServer;
    int server_fd;
    int port;
    const int maxClients;
    const std::string startString = "START_TRANSMISSION";
    int flag = 0;

    static uint64_t htonll(uint64_t value)
    {
        static const int num = 42;
        if (*reinterpret_cast<const char *>(&num) == num)
        {
            const uint32_t high_part = htonl(static_cast<uint32_t>(value >> 32));
            const uint32_t low_part = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));
            return (static_cast<uint64_t>(low_part) << 32) | high_part;
        }
        else
        {
            return value;
        }
    }

    static uint64_t ntohll(uint64_t value)
    {
        return htonll(value);
    }

    static std::vector<char> serialize(const std::vector<double> &data)
    {
        std::vector<char> serializedData(data.size() * sizeof(double));
        char *buffer = serializedData.data();

        for (double value : data)
        {
            uint64_t netValue = htonll(*reinterpret_cast<uint64_t *>(&value));
            std::memcpy(buffer, &netValue, sizeof(uint64_t));
            buffer += sizeof(uint64_t);
        }

        return serializedData;
    }

    static std::vector<double> deserialize(const std::vector<char> &serializedData)
    {
        std::vector<double> data(serializedData.size() / sizeof(double));
        const char *buffer = serializedData.data();

        for (double &value : data)
        {
            uint64_t netValue;
            std::memcpy(&netValue, buffer, sizeof(uint64_t));
            netValue = ntohll(netValue);
            value = *reinterpret_cast<double *>(&netValue);
            buffer += sizeof(uint64_t);
        }

        return data;
    }

    void handleClient(int client_socket)
    {
        unsigned int count = 0;

        while (!stopServer)
        {
            if (!flag)
            {
                flag = 1;
                char buffer[256];
                memset(buffer, 0, sizeof(buffer));
                recv(client_socket, buffer, sizeof(buffer), 0);
                std::string receivedString(buffer);
                // std::cout << "Received message: " << receivedString << std::endl;
                if (receivedString != startString)
                {
                    std::cerr << "Invalid start string. Closing connection." << std::endl;
                    close(client_socket);
                    return;
                }
            }
            const auto start = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::system_clock::now();
            auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
            auto epoch = now_ms.time_since_epoch();
            auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
            count++;
            double val = sin((count * 0.002));
            // change this part
            /*
            torques as var
            new function for simu
            new buffer
            int valread1 = read(simu_socket, buffer.data(), buffer.size());
            if (valread1 > 0) {
                torques = deserialize(buffer);
                printTimeAndData("Torques received", torques);

                serializedData = serialize(torques);
                send(arm_socket, serializedData.data(), serializedData.size(), 0);
                printTimeAndData("Torques sent", torques);
            }
            */

            std::vector<double> dataToSend = {0, 0, 0, 0, val, val, 0};
            std::vector<char> serializedData = serialize(dataToSend);
            send(client_socket, serializedData.data(), serializedData.size(), 0);

            const auto end = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> elapsed = end - start;
            std::cout << "SLEEP FOR " << elapsed.count() << " ms"
                      << "| DATA: " << dataToSend.at(4) << std::endl;
            // printf("%lld | Data sent at count %i is %f\n", value.count(), count, dataToSend.at(4));

            flag = 0;
        }

        close(client_socket);
    }

    void handleSimulink(int simu_socket) {
        close(simu_socket);
    }

    std::vector<double> readFromClient(int client_socket)
    {
        // char buffer[1024] = {0};
        // int valread = read(client_socket, buffer, 1024);
        // std::string receivedMessage(buffer);
        // std::cout << "Received message: " << receivedMessage << std::endl;

        std::vector<char> buffer1(256);
        int valread = read(client_socket, buffer1.data(), 56);
        std::cout << "Received " << valread << " bytes" << std::endl;
        return deserialize(buffer1);
    }

public:
    MultiServer(int port_, int maxClients_) : stopServer(false), port(port_), maxClients(maxClients_), server_fd(-1) {}

    ~MultiServer()
    {
        if (server_fd != -1)
        {
            close(server_fd);
        }
    }

    void start()
    {
        struct sockaddr_in address;
        int opt = 1;

        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 3) < 0)
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        std::vector<std::thread> clientThreads;

        while (clientThreads.size() < maxClients)
        {
            int new_socket;
            int addrlen = sizeof(address);

            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            clientThreads.emplace_back(&MultiServer::handleClient, this, new_socket);
        }

        // Keep the server running for a while
        std::this_thread::sleep_for(std::chrono::seconds(10));

        // Wait for all client threads to finish
        for (auto &thread : clientThreads)
        {
            thread.join();
        }
    }
};

int main()
{
    MultiServer server(9090, 1);
    server.start();
    return 0;
}
