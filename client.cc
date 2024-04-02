#include <iostream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>
#include <ctime>
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


void client(const string& server_ip, int port) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return;
    }

    while (true) {
        vector<char> buffer(56);
        int valread = read(sock, buffer.data(), 56);
        if (valread > 0) {
            vector<double> receivedData = deserialize(buffer);
            auto now = chrono::system_clock::now();
            auto currentTime = chrono::system_clock::to_time_t(now);
            auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
            tm* localTime = localtime(&currentTime);

        cout << "Data received at: ";
        cout << put_time(localTime, "%Y-%m-%d %H:%M:%S");
        cout << '.' << std::setfill('0') << std::setw(3) << ms.count() << ": ";
        for (double value : receivedData) {
            cout << value << " ";
        }
        cout << endl;
        }
        
    }

    close(sock);
}

int main() {
    client("127.0.0.1", 9090); //host ip or replace with other non-local ip 
    return 0;
}