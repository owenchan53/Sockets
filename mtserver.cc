#include <iostream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <chrono>
#include <ctime>
#include <thread>
using namespace std;

// Helper function for htonll and ntohll
uint64_t htonll(uint64_t value) {
    static const int num = 42;

    // Check the endianness
    if (*reinterpret_cast<const char*>(&num) == num) {
        const uint32_t high_part = htonl(static_cast<uint32_t>(value >> 32));
        const uint32_t low_part = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

        return (static_cast<uint64_t>(low_part) << 32) | high_part;
    } else {
        return value;
    }
}

uint64_t ntohll(uint64_t value) {
    // htonll and ntohll are symmetric
    return htonll(value);
}

// Serialize array of doubles
vector<char> serialize(const vector<double>& data) {
    std::vector<char> serializedData(data.size() * sizeof(double));
    char* buffer = serializedData.data();

    for (double value : data) {
        uint64_t netValue = htonll(*reinterpret_cast<uint64_t*>(&value));
        std::memcpy(buffer, &netValue, sizeof(uint64_t));
        buffer += sizeof(uint64_t);
    }

    return serializedData;
}

// Deserialize array of doubles
vector<double> deserialize(const vector<char>& serializedData) {
    std::vector<double> data(serializedData.size() / sizeof(double));
    const char* buffer = serializedData.data();

    for (double& value : data) {
        uint64_t netValue;
        std::memcpy(&netValue, buffer, sizeof(uint64_t));
        netValue = ntohll(netValue);
        value = *reinterpret_cast<double*>(&netValue);
        buffer += sizeof(uint64_t);
    }

    return data;
}

void printTimeAndData(const string& prefix, const vector<double>& data) {
    auto now = chrono::system_clock::now();
    auto currentTime = chrono::system_clock::to_time_t(now);
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    tm* localTime = localtime(&currentTime);

    cout << prefix << " at: " << put_time(localTime, "%Y-%m-%d %H:%M:%S");
    cout << '.' << setfill('0') << setw(3) << ms.count() << ": ";
    for (double value : data) { cout << value << " "; }
    cout << endl;
}

void server(int port) {
    int server_fd, arm_socket, simu_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

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

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((arm_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    if ((simu_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    vector<double> torques(7,2);
    vector<double> armState(7,0);
    vector<char> buffer(56);

    while (true) {
        int valread = read(arm_socket, buffer.data(), buffer.size());
        if (valread > 0) {
            armState = deserialize(buffer);
            printTimeAndData("Arm state received", armState);
  
            vector<char> serializedData = serialize(armState);
            send(simu_socket, serializedData.data(), serializedData.size(), 0);

            int valread1 = read(simu_socket, buffer.data(), buffer.size());
            if (valread1 > 0) {
                torques = deserialize(buffer);
                printTimeAndData("Torques received", torques);

                serializedData = serialize(torques);
                send(arm_socket, serializedData.data(), serializedData.size(), 0);
                printTimeAndData("Torques sent", torques);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    close(arm_socket);
    close(simu_socket);
    close(server_fd);
}

int main() {
    server(9090); 
    return 0;
}