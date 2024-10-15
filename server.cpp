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

// Global variables
HashTable<std::string, Entry*> db;
std::vector<Connection*> fd2conn;

// Function prototypes
void handle_command(const std::vector<std::string>& cmd, std::string& out);
void out_string(std::string& out, const std::string& str);
void out_nil(std::string& out);
void out_int(std::string& out, int64_t val);
void out_error(std::string& out, const std::string& msg);
void out_ok(std::string& out);
void out_array(std::string& out, const std::vector<std::string>& arr);
void out_double(std::string& out, double val);
int parse_request(const uint8_t* data, size_t len, std::vector<std::string>& out);
void close_connection(Connection* conn);
void handle_connection_io(Connection* conn);
void accept_new_connection(int listen_fd);
void handle_read(Connection* conn);
void handle_write(Connection* conn);

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }

    int val = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);
    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("listen");
        exit(1);
    }

    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    while (true) {
        std::vector<pollfd> pollfds;
        pollfds.push_back({listen_fd, POLLIN, 0});
        for (Connection* conn : fd2conn) {
            if (conn) {
                pollfd pfd = {conn->fd, 0, 0};
                if (conn->state == STATE_REQ) {
                    pfd.events = POLLIN;
                } else if (conn->state == STATE_RES) {
                    pfd.events = POLLOUT;
                }
                pfd.events |= POLLERR;
                pollfds.push_back(pfd);
            }
        }

        int rv = poll(pollfds.data(), pollfds.size(), 1000);
        if (rv < 0) {
            perror("poll");
            exit(1);
        }

        size_t idx = 0;
        if (pollfds[idx++].revents & POLLIN) {
            accept_new_connection(listen_fd);
        }

        for (; idx < pollfds.size(); ++idx) {
            pollfd& pfd = pollfds[idx];
            Connection* conn = fd2conn[pfd.fd];
            if (!conn) continue;
            if (pfd.revents & (POLLIN | POLLOUT | POLLERR)) {
                handle_connection_io(conn);
                if (conn->state == STATE_END) {
                    close_connection(conn);
                    fd2conn[pfd.fd] = nullptr;
                    delete conn;
                }
            }
        }
    }
    return 0;
}

void accept_new_connection(int listen_fd) {
    sockaddr_in client_addr;
    socklen_t socklen = sizeof(client_addr);
    int conn_fd = accept(listen_fd, (sockaddr*)&client_addr, &socklen);
    if (conn_fd >= 0) {
        fcntl(conn_fd, F_SETFL, O_NONBLOCK);
        Connection* conn = new Connection(conn_fd);
        if (fd2conn.size() <= (size_t)conn_fd) {
            fd2conn.resize(conn_fd + 1, nullptr);
        }
        fd2conn[conn_fd] = conn;
    }
}

void close_connection(Connection* conn) {
    close(conn->fd);
}

void handle_connection_io(Connection* conn) {
    if (conn->state == STATE_REQ) {
        handle_read(conn);
    } else if (conn->state == STATE_RES) {
        handle_write(conn);
    }
}

void handle_read(Connection* conn) {
    while (true) {
        uint8_t buf[4096];
        ssize_t n = read(conn->fd, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EAGAIN) {
                break;
            } else {
                perror("read");
                conn->state = STATE_END;
                break;
            }
        } else if (n == 0) {
            conn->state = STATE_END;
            break;
        } else {
            conn->rbuf.insert(conn->rbuf.end(), buf, buf + n);
            while (true) {
                if (conn->rbuf.size() < 4) {
                    break;
                }
                uint32_t len = 0;
                memcpy(&len, &conn->rbuf[0], 4);
                if (len > Connection::k_max_msg) {
                    std::cerr << "Message too long\n";
                    conn->state = STATE_END;
                    break;
                }
                if (conn->rbuf.size() < 4 + len) {
                    break;
                }
                std::vector<std::string> cmd;
                if (parse_request(&conn->rbuf[4], len, cmd) != 0) {
                    std::cerr << "Bad request\n";
                    conn->state = STATE_END;
                    break;
                }
                std::string response;
                handle_command(cmd, response);
                uint32_t wlen = response.size();
                conn->wbuf.resize(4 + wlen);
                memcpy(&conn->wbuf[0], &wlen, 4);
                memcpy(&conn->wbuf[4], response.data(), wlen);
                conn->wbuf_sent = 0;
                conn->state = STATE_RES;
                conn->rbuf.erase(conn->rbuf.begin(), conn->rbuf.begin() + 4 + len);
                break;
            }
        }
    }
}

void handle_write(Connection* conn) {
    while (conn->wbuf_sent < conn->wbuf.size()) {
        ssize_t n = write(conn->fd, &conn->wbuf[conn->wbuf_sent], conn->wbuf.size() - conn->wbuf_sent);
        if (n < 0) {
            if (errno == EAGAIN) {
                break;
            } else {
                perror("write");
                conn->state = STATE_END;
                break;
            }
        } else {
            conn->wbuf_sent += n;
        }
    }
    if (conn->wbuf_sent == conn->wbuf.size()) {
        conn->wbuf.clear();
        conn->wbuf_sent = 0;
        conn->state = STATE_REQ;
    }
}

void handle_command(const std::vector<std::string>& cmd, std::string& out) {
    if (cmd.empty()) {
        out_error(out, "Empty command");
        return;
    }
    const std::string& command = cmd[0];
    if (command == "get") {
        if (cmd.size() != 2) {
            out_error(out, "Invalid number of arguments for 'get'");
            return;
        }
        Entry* entry = db.get(cmd[1]);
        if (entry && entry->type == Entry::STRING) {
            out_string(out, entry->str_value);
        } else {
            out_nil(out);
        }
    } else if (command == "set") {
        if (cmd.size() != 3) {
            out_error(out, "Invalid number of arguments for 'set'");
            return;
        }
        Entry* entry = db.get(cmd[1]);
        if (entry) {
            if (entry->type != Entry::STRING) {
                out_error(out, "Wrong type");
                return;
            }
            entry->str_value = cmd[2];
        } else {
            entry = new Entry(cmd[1], cmd[2]);
            db.put(cmd[1], entry);
        }
        out_ok(out);
    } else if (command == "del") {
        if (cmd.size() != 2) {
            out_error(out, "Invalid number of arguments for 'del'");
            return;
        }
        db.remove(cmd[1]);
        out_int(out, 1);
    } else if (command == "keys") {
        if (cmd.size() != 1) {
            out_error(out, "Invalid number of arguments for 'keys'");
            return;
        }
        std::vector<std::string> keys = db.keys();
        out_array(out, keys);
    } else if (command == "zadd") {
        if (cmd.size() != 4) {
            out_error(out, "Invalid number of arguments for 'zadd'");
            return;
        }
        const std::string& key = cmd[1];
        double score = std::stod(cmd[2]);
        const std::string& member = cmd[3];
        
        Entry* entry = db.get(key);
        if (!entry) {
            entry = new Entry(key, "");
            entry->type = Entry::ZSET;
            entry->zset_value = new ZSet();
            db.put(key, entry);
        }
        if (entry->type != Entry::ZSET) {
            out_error(out, "Wrong type");
            return;
        }
        bool added = entry->zset_value->zadd(member, score);
        out_int(out, added ? 1 : 0);
    } else if (command == "zrem") {
        if (cmd.size() != 3) {
            out_error(out, "Invalid number of arguments for 'zrem'");
            return;
        }
        const std::string& key = cmd[1];
        const std::string& member = cmd[2];
        
        Entry* entry = db.get(key);
        if (!entry || entry->type != Entry::ZSET) {
            out_error(out, "Wrong type or key does not exist");
            return;
        }
        bool removed = entry->zset_value->zrem(member);
        out_int(out, removed ? 1 : 0);
    } else if (command == "zscore") {
        if (cmd.size() != 3) {
            out_error(out, "Invalid number of arguments for 'zscore'");
            return;
        }
        const std::string& key = cmd[1];
        const std::string& member = cmd[2];
        
        Entry* entry = db.get(key);
        if (!entry || entry->type != Entry::ZSET) {
            out_error(out, "Wrong type or key does not exist");
            return;
        }
        double score;
        if (entry->zset_value->zscore(member, score)) {
            out_double(out, score);
        } else {
            out_nil(out);
        }
    } else if (command == "zquery") {
        if (cmd.size() != 6) {
            out_error(out, "Invalid number of arguments for 'zquery'");
            return;
        }
        const std::string& key = cmd[1];
        double min_score = std::stod(cmd[2]);
        const std::string& min_member = cmd[3];
        int offset = std::stoi(cmd[4]);
        int limit = std::stoi(cmd[5]);

        Entry* entry = db.get(key);
        if (!entry || entry->type != Entry::ZSET) {
            out_error(out, "Wrong type or key does not exist");
            return;
        }
        std::vector<std::pair<std::string, double>> result = entry->zset_value->zquery(min_score, min_member, offset, limit);
        out.push_back(SER_ARR);
        uint32_t len = result.size() * 2;
        out.append((char*)&len, 4);
        for (const auto& pair : result) {
            out_string(out, pair.first);
            out_double(out, pair.second);
        }
    } else {
        out_error(out, "Unknown command");
    }
}

// Serialization functions
void out_string(std::string& out, const std::string& str) {
    out.push_back(SER_STR);
    uint32_t len = str.size();
    out.append((char*)&len, 4);
    out.append(str);
}

void out_nil(std::string& out) {
    out.push_back(SER_NIL);
}

void out_int(std::string& out, int64_t val) {
    out.push_back(SER_INT);
    out.append((char*)&val, 8);
}

void out_error(std::string& out, const std::string& msg) {
    out.push_back(SER_ERR);
    uint32_t len = msg.size();
    out.append((char*)&len, 4);
    out.append(msg);
}

void out_ok(std::string& out) {
    out_string(out, "OK");
}

void out_array(std::string& out, const std::vector<std::string>& arr) {
    out.push_back(SER_ARR);
    uint32_t len = arr.size();
    out.append((char*)&len, 4);
    for (const std::string& s : arr) {
        out_string(out, s);
    }
}

void out_double(std::string& out, double val) {
    out.push_back(SER_DBL);
    out.append((char*)&val, 8);
}

// Parsing request from client
int parse_request(const uint8_t* data, size_t len, std::vector<std::string>& out) {
    if (len < 4) {
        return -1;
    }
    uint32_t argc = 0;
    memcpy(&argc, data, 4);
    if (argc > 1024) {
        return -1;
    }
    size_t pos = 4;
    for (uint32_t i = 0; i < argc; ++i) {
        if (pos + 4 > len) {
            return -1;
        }
        uint32_t arg_len = 0;
        memcpy(&arg_len, data + pos, 4);
        pos += 4;
        if (pos + arg_len > len) {
            return -1;
        }
        out.push_back(std::string((char*)data + pos, arg_len));
        pos += arg_len;
    }
    if (pos != len) {
        return -1;
    }
    return 0;
}
