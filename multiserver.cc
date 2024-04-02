#include <atomic>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <thread>
#include <chrono>
#include <cmath>
#include <ctime>
using namespace std;

atomic<bool> stopServer(false);

// Helper function for htonll and ntohll
uint64_t htonll(uint64_t value) {
    static const int num = 42;

    // Check the endianness
    if (*reinterpret_cast<const char *>(&num) == num) {
        const uint32_t high_part = htonl(static_cast<uint32_t>(value >> 32));
        const uint32_t low_part = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

        return (static_cast<uint64_t>(low_part) << 32) | high_part;
    }
    else {
        return value;
    }
}

uint64_t ntohll(uint64_t value) { return htonll(value); }

vector<char> serialize(const vector<double> &data) {
    vector<char> serializedData(data.size() * sizeof(double));
    char *buffer = serializedData.data();

    for (double value : data) {
        uint64_t netValue = htonll(*reinterpret_cast<uint64_t *>(&value));
        memcpy(buffer, &netValue, sizeof(uint64_t));
        buffer += sizeof(uint64_t);
    }

    return serializedData;
}

// Deserialize array of doubles
vector<double> deserialize(const vector<char> &serializedData) {
    std::vector<double> data(serializedData.size() / sizeof(double));
    const char *buffer = serializedData.data();

    for (double &value : data) {
        uint64_t netValue;
        std::memcpy(&netValue, buffer, sizeof(uint64_t));
        netValue = ntohll(netValue);
        value = *reinterpret_cast<double *>(&netValue);
        buffer += sizeof(uint64_t);
    }

    return data;
}

void handleClient(int client_socket) {
    bool flag = false;
    while (!stopServer) {
        if (flag == false) {
            flag = true;
            this_thread::sleep_for(chrono::seconds(5));
        }
        double count = chrono::duration_cast<chrono::milliseconds>
            (chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
        double val = sin(count);
        vector<double> dataToSend = {0, 0, 0, 0, 0, val, 0};
        // dataToSend.at(5) = val;
        vector<char> serializedData = serialize(dataToSend);
        send(client_socket, serializedData.data(), serializedData.size(), 0);
        this_thread::sleep_for(chrono::milliseconds(5));
        auto now = chrono::system_clock::now();
        auto currentTime = chrono::system_clock::to_time_t(now);
        auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
        tm* localTime = localtime(&currentTime);
        cout << "Data sent at count ";
        cout << put_time(localTime, "%Y-%m-%d %H:%M:%S");
        cout << '.' << std::setfill('0') << std::setw(3) << ms.count() << ": ";
        cout << " is " << dataToSend.at(5) << endl;
    }
    // close(client_socket);
}

void server(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR 
        | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    vector<std::thread> clientThreads;
    const int maxClients = 2; // Change this to the desired number of clients

    while (clientThreads.size() < maxClients) {
        int new_socket;
        int addrlen = sizeof(address);

        if ((new_socket = accept(server_fd, (struct sockaddr *)
            &address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        clientThreads.emplace_back(handleClient, new_socket);
    }

    // Keep the server running for a while
    this_thread::sleep_for(std::chrono::seconds(10));

    // Signal to stop the server
    stopServer = true;

    // Wait for all client threads to finish
    for (auto &thread : clientThreads) {
        thread.join();
    }

    close(server_fd);
}

int main() {
    server(9090);
    return 0;
}