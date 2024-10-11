
#pragma once

#include <string>
#include <unistd.h>
#include <sys/socket.h>

class NetworkSocket {
public:

    NetworkSocket();
    ~NetworkSocket();

    bool okay() const { return file_descriptor < 0; }

    bool bind(uint16_t port);
    bool listen(int backlog = SOMAXCONN);
    bool connect(const std::string& ip, uint16_t port);
    int accept();
    void write(const std::string& msg);
    std::string read(size_t buffer_size);
    bool poll(std::vector<struct pollfd>& fds, int timeout);

    bool set_sock_option(int level, int option_name, int flag);
    bool set_non_blocking(bool enable);
    void set_read_timeout(int seconds, int microseconds);
    void set_write_timeout(int seconds, int microseconds);

    std::string get_local_ip();
    std::string get_peer_ip();
    uint16_t get_local_port();
    uint16_t get_peer_port();

    int get_fd() const { return file_descriptor; }

protected:
    int file_descriptor;

};