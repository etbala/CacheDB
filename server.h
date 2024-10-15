
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <cassert>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#include "hashtable.h"
#include "zset.h"

// Serialization codes
enum {
    SER_STR = 0,
    SER_NIL = 1,
    SER_INT = 2,
    SER_ERR = 3,
    SER_ARR = 4,
    SER_DBL = 5,
};

// Entry class representing a key-value pair
class Entry {
public:
    enum Type {
        STRING,
        ZSET
    };
    std::string key;
    Type type;
    std::string str_value; // For STRING type
    ZSet* zset_value; // For ZSET type

    Entry(const std::string& k, const std::string& val)
        : key(k), type(STRING), str_value(val), zset_value(nullptr) {}
    ~Entry() {
        if (type == ZSET && zset_value) {
            delete zset_value;
        }
    }
};

enum ConnectionState {
    STATE_REQ,
    STATE_RES,
    STATE_END
};

// Connection struct representing a client connection
struct Connection {
    int fd;
    ConnectionState state;
    std::vector<uint8_t> rbuf;
    std::vector<uint8_t> wbuf;
    size_t wbuf_sent;

    static const size_t k_max_msg = 4096;

    Connection(int fd_) : fd(fd_), state(STATE_REQ), wbuf_sent(0) {
        rbuf.reserve(4 + k_max_msg);
        wbuf.reserve(4 + k_max_msg);
    }
};

