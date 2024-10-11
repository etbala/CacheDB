
#pragma once

#include <vector>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "socket.h"

class PollManager {
public:
    PollManager(NetworkSocket socket) : listening_socket(socket) {}

    auto add_fd(int fd, short events) -> void;
    auto poll(int timeout) -> int;
    auto get_poll_fds() -> std::vector<struct pollfd>& { return poll_fds; }
    
private:
    NetworkSocket listening_socket;

    std::vector<struct pollfd> poll_fds;
};