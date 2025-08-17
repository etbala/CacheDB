#include "client/transport.h"
#include "client/util.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>

TcpTransport::TcpTransport() : sockfd_(-1) {}
TcpTransport::~TcpTransport() { close(); }

void TcpTransport::connect(const std::string& host, uint16_t port) {
    sockfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) die("socket failed");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        die("Invalid address / Address not supported");
    }
    if (::connect(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        die("Connection failed");
    }
}

void TcpTransport::send_all(const uint8_t* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(sockfd_, data + sent, len - sent, 0);
        if (n < 0) die("send failed");
        sent += static_cast<size_t>(n);
    }
}

void TcpTransport::recv_all(uint8_t* data, size_t len) {
    size_t recvd = 0;
    while (recvd < len) {
        ssize_t n = ::recv(sockfd_, data + recvd, len - recvd, 0);
        if (n <= 0) die("recv failed");
        recvd += static_cast<size_t>(n);
    }
}

void TcpTransport::close() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}
