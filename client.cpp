
#include <iostream>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <sstream>

void die(const std::string& message) {
    int err = errno;
    std::cerr << "[" << err << "] " << message << std::endl;
    std::exit(EXIT_FAILURE);
}

// Serialization codes
enum {
    SER_STR = 0,
    SER_NIL = 1,
    SER_INT = 2,
    SER_ERR = 3,
    SER_ARR = 4,
    SER_DBL = 5,
};

// Serialize the request according to the server's protocol
void serialize_request(const std::vector<std::string>& cmd, std::vector<uint8_t>& out) {
    uint32_t argc = cmd.size();
    uint32_t data_len = 4; // length of argc
    for (const std::string& arg : cmd) {
        data_len += 4; // length of arg_len
        data_len += arg.size(); // length of argument data
    }
    uint32_t total_len = data_len; // total length of data after the initial len field

    out.resize(4 + data_len); // 4 bytes for len, plus data_len bytes for data
    uint32_t offset = 0;

    // Write total_len into the first 4 bytes
    memcpy(&out[offset], &total_len, 4);
    offset += 4;

    // Write argc
    memcpy(&out[offset], &argc, 4);
    offset += 4;

    for (const std::string& arg : cmd) {
        uint32_t arg_len = arg.size();
        memcpy(&out[offset], &arg_len, 4);
        offset += 4;
        memcpy(&out[offset], arg.data(), arg_len);
        offset += arg_len;
    }
}

void deserialize_response(const std::vector<uint8_t>& in, size_t& offset) {
    if (offset >= in.size()) {
        die("Response parsing error: offset out of bounds");
    }
    uint8_t type = in[offset++];
    switch (type) {
        case SER_STR: {
            if (offset + 4 > in.size()) {
                die("Response parsing error: string length");
            }
            uint32_t len = 0;
            memcpy(&len, &in[offset], 4);
            offset += 4;
            if (offset + len > in.size()) {
                die("Response parsing error: string data");
            }
            std::string str((char*)&in[offset], len);
            offset += len;
            std::cout << str << std::endl;
            break;
        }
        case SER_INT: {
            if (offset + 8 > in.size()) {
                die("Response parsing error: int data");
            }
            int64_t val = 0;
            memcpy(&val, &in[offset], 8);
            offset += 8;
            std::cout << val << std::endl;
            break;
        }
        case SER_DBL: {
            if (offset + 8 > in.size()) {
                die("Response parsing error: double data");
            }
            double val = 0;
            memcpy(&val, &in[offset], 8);
            offset += 8;
            std::cout << val << std::endl;
            break;
        }
        case SER_NIL: {
            std::cout << "(nil)" << std::endl;
            break;
        }
        case SER_ERR: {
            if (offset + 4 > in.size()) {
                die("Response parsing error: error length");
            }
            uint32_t len = 0;
            memcpy(&len, &in[offset], 4);
            offset += 4;
            if (offset + len > in.size()) {
                die("Response parsing error: error data");
            }
            std::string str((char*)&in[offset], len);
            offset += len;
            std::cerr << "(error) " << str << std::endl;
            break;
        }
        case SER_ARR: {
            if (offset + 4 > in.size()) {
                die("Response parsing error: array length");
            }
            uint32_t len = 0;
            memcpy(&len, &in[offset], 4);
            offset += 4;
            for (uint32_t i = 0; i < len; ++i) {
                deserialize_response(in, offset);
            }
            break;
        }
        default:
            die("Response parsing error: unknown type");
    }
}

int main() {
    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        die("socket failed");
    }

    // Server address
    sockaddr_in serv_addr = {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1234);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        die("Invalid address / Address not supported");
    }

    // Connect to the server
    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        die("Connection failed");
    }

    std::string line;
    while (std::cout << "> ", std::getline(std::cin, line)) {
        // Split line into arguments
        std::istringstream iss(line);
        std::vector<std::string> cmd;
        std::string arg;
        while (iss >> arg) {
            cmd.push_back(arg);
        }
        if (cmd.empty()) {
            continue;
        }

        // Serialize the request
        std::vector<uint8_t> request;
        serialize_request(cmd, request);

        // Send the request
        size_t total_sent = 0;
        while (total_sent < request.size()) {
            ssize_t n = send(sockfd, &request[total_sent], request.size() - total_sent, 0);
            if (n < 0) {
                die("send failed");
            }
            total_sent += n;
        }

        // Receive the response header (4 bytes indicating the length)
        std::vector<uint8_t> response_header(4);
        ssize_t n = recv(sockfd, &response_header[0], 4, MSG_WAITALL);
        if (n <= 0) {
            die("recv failed");
        }
        uint32_t resp_len = 0;
        memcpy(&resp_len, &response_header[0], 4);
        if (resp_len > 10 * 1024 * 1024) {
            die("Response too large");
        }

        // Receive the response body
        std::vector<uint8_t> response_body(resp_len);
        size_t total_received = 0;
        while (total_received < resp_len) {
            n = recv(sockfd, &response_body[total_received], resp_len - total_received, 0);
            if (n <= 0) {
                die("recv failed");
            }
            total_received += n;
        }

        // Parse and print the response
        size_t offset = 0;
        deserialize_response(response_body, offset);
    }

    close(sockfd);
    return 0;
}
