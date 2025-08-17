#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "server/hashtable.h"
#include "server/entry.h"

enum ConnectionState {
    STATE_REQ,
    STATE_RES,
    STATE_END
};

struct Connection {
    int fd;
    ConnectionState state;
    std::vector<uint8_t> rbuf;
    std::vector<uint8_t> wbuf;
    size_t wbuf_sent;

    static const size_t k_max_msg = 4096;

    explicit Connection(int fd_)
        : fd(fd_), state(STATE_REQ), wbuf_sent(0) {
        rbuf.reserve(4 + k_max_msg);
        wbuf.reserve(4 + k_max_msg);
    }
};

class Server {
public:
    Server();
    ~Server();

    // Runs the poll loop on an already-bound+listening socket.
    void run(int listen_fd);

    void handle_command(const std::vector<std::string>& cmd, std::string& out);

private:
    void accept_new_connection(int listen_fd);
    void close_connection(Connection* conn);
    void handle_connection_io(Connection* conn);
    void handle_read(Connection* conn);
    void handle_write(Connection* conn);

private:
    HashTable<std::string, Entry*> db_;
    std::vector<Connection*> fd2conn_;
};
