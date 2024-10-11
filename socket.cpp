
#include "socket.h"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <poll.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>

NetworkSocket::NetworkSocket()
{
    file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (file_descriptor < 0) {
        throw std::runtime_error("socket() failed");
    }
    
    int val = 1;
    if (setsockopt(file_descriptor, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        throw std::runtime_error("setsockopt() failed");
    }
}

NetworkSocket::~NetworkSocket()
{
    if (file_descriptor >= 0) {
        close(file_descriptor);
    }
}

auto NetworkSocket::bind(uint16_t port) -> bool
{
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // wildcard address 0.0.0.0

    int rv = ::bind(file_descriptor, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    return !rv;
}

auto NetworkSocket::listen(int backlog) -> bool
{
    int rv = ::listen(file_descriptor, backlog);
    return rv >= 0;
}

auto NetworkSocket::accept() -> int
{
    sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = ::accept(file_descriptor, reinterpret_cast<sockaddr*>(&client_addr), &socklen);
    return connfd;
}

auto NetworkSocket::connect(const std::string& ip, uint16_t port) -> bool
{
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    int rv = inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    if (rv <= 0) {
        return false;
    }

    rv = ::connect(file_descriptor, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (rv < 0) {
        return false;
    }

    return true;
}

auto NetworkSocket::write(const std::string& msg) -> void
{
    if (::write(file_descriptor, msg.c_str(), msg.size()) < 0) {
        throw std::runtime_error("write() failed");
    }
}

auto NetworkSocket::read(size_t buffer_size) -> std::string
{
    std::vector<char> rbuf(buffer_size, 0);
    ssize_t n = ::read(file_descriptor, rbuf.data(), buffer_size - 1);
    if (n < 0) {
        throw std::runtime_error("read() failed");
    }
    return std::string(rbuf.data());
}

auto poll(std::vector<struct pollfd>& fds, int timeout) -> int
{
    int ret = ::poll(fds.data(), fds.size(), timeout);
    if (ret < 0) {
        return false;
    }

    return true;
    
    // struct pollfd pfd;
    // pfd.fd = file_descriptor;
    // pfd.events = events;

    // int ret = ::poll(&pfd, 1, timeout);
    // if (ret < 0) {
    //     return false;
    // }
    
    // return true;
}

auto NetworkSocket::set_sock_option(int level, int option_name, int flag) -> bool
{
    return setsockopt(file_descriptor, level, option_name, &flag, sizeof(flag)) >= 0;
}

auto NetworkSocket::set_non_blocking(bool enable) -> bool
{
    int flags = fcntl(file_descriptor, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }

    flags = (enable) ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);

    int ret = fcntl(file_descriptor, F_SETFL, flags) < 0;
    if (ret < 0) {
        return false;
    }

    return true;
}

auto NetworkSocket::set_read_timeout(int seconds, int microseconds) -> void
{
    timeval timeout = {};
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;
    if (setsockopt(file_descriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("setsockopt(SO_RCVTIMEO) failed: " + std::string(strerror(errno)));
    }
}

auto NetworkSocket::set_write_timeout(int seconds, int microseconds) -> void
{
    timeval timeout = {};
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;
    if (setsockopt(file_descriptor, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("setsockopt(SO_SNDTIMEO) failed: " + std::string(strerror(errno)));
    }
}

auto NetworkSocket::get_local_ip() -> std::string
{
    sockaddr_in addr = {};
    socklen_t len = sizeof(addr);
    if (getsockname(file_descriptor, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
        throw std::runtime_error("getsockname() failed: " + std::string(strerror(errno)));
    }
    char ip[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip))) {
        throw std::runtime_error("inet_ntop() failed: " + std::string(strerror(errno)));
    }
    return std::string(ip);
}

auto NetworkSocket::get_local_port() -> uint16_t
{
    sockaddr_in addr = {};
    socklen_t len = sizeof(addr);
    if (getsockname(file_descriptor, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
        throw std::runtime_error("getsockname() failed: " + std::string(strerror(errno)));
    }
    return ntohs(addr.sin_port);
}

auto NetworkSocket::get_peer_ip() -> std::string
{
    sockaddr_in addr = {};
    socklen_t len = sizeof(addr);
    if (getpeername(file_descriptor, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
        throw std::runtime_error("getpeername() failed: " + std::string(strerror(errno)));
    }
    char ip[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip))) {
        throw std::runtime_error("inet_ntop() failed: " + std::string(strerror(errno)));
    }
    return std::string(ip);
}

auto NetworkSocket::get_peer_port() -> uint16_t
{
    sockaddr_in addr = {};
    socklen_t len = sizeof(addr);
    if (getpeername(file_descriptor, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
        throw std::runtime_error("getpeername() failed: " + std::string(strerror(errno)));
    }
    return ntohs(addr.sin_port);
}