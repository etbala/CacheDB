#include "server/server.h"
#include "server/protocol.h"

#include <iostream>
#include <cassert>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

Server::Server() = default;
Server::~Server() = default;

void Server::run(int listen_fd) {
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    while (true) {
        std::vector<pollfd> pollfds;
        pollfds.push_back({listen_fd, POLLIN, 0});
        for (Connection* conn : fd2conn_) {
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

        int rv = ::poll(pollfds.data(), pollfds.size(), 1000);
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
            Connection* conn = fd2conn_[pfd.fd];
            if (!conn) continue;
            if (pfd.revents & (POLLIN | POLLOUT | POLLERR)) {
                handle_connection_io(conn);
                if (conn->state == STATE_END) {
                    close_connection(conn);
                    fd2conn_[pfd.fd] = nullptr;
                    delete conn;
                }
            }
        }
    }
}

void Server::accept_new_connection(int listen_fd) {
    sockaddr_in client_addr{};
    socklen_t socklen = sizeof(client_addr);
    int conn_fd = ::accept(listen_fd, (sockaddr*)&client_addr, &socklen);
    if (conn_fd >= 0) {
        fcntl(conn_fd, F_SETFL, O_NONBLOCK);
        Connection* conn = new Connection(conn_fd);
        if (fd2conn_.size() <= (size_t)conn_fd) {
            fd2conn_.resize(conn_fd + 1, nullptr);
        }
        fd2conn_[conn_fd] = conn;
    }
}

void Server::close_connection(Connection* conn) {
    ::close(conn->fd);
}

void Server::handle_connection_io(Connection* conn) {
    if (conn->state == STATE_REQ) {
        handle_read(conn);
    } else if (conn->state == STATE_RES) {
        handle_write(conn);
    }
}

void Server::handle_read(Connection* conn) {
    while (true) {
        uint8_t buf[4096];
        ssize_t n = ::read(conn->fd, buf, sizeof(buf));
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
                if (conn->rbuf.size() < 4) break;
                uint32_t len = 0;
                std::memcpy(&len, &conn->rbuf[0], 4);
                if (len > Connection::k_max_msg) {
                    std::cerr << "Message too long\n";
                    conn->state = STATE_END;
                    break;
                }
                if (conn->rbuf.size() < 4 + len) break;

                std::vector<std::string> cmd;
                if (parse_request(&conn->rbuf[4], len, cmd) != 0) {
                    std::cerr << "Bad request\n";
                    conn->state = STATE_END;
                    break;
                }
                std::string response;
                handle_command(cmd, response);

                uint32_t wlen = static_cast<uint32_t>(response.size());
                conn->wbuf.resize(4 + wlen);
                std::memcpy(&conn->wbuf[0], &wlen, 4);
                std::memcpy(&conn->wbuf[4], response.data(), wlen);
                conn->wbuf_sent = 0;
                conn->state = STATE_RES;

                conn->rbuf.erase(conn->rbuf.begin(), conn->rbuf.begin() + 4 + len);
                break;
            }
        }
    }
}

void Server::handle_write(Connection* conn) {
    while (conn->wbuf_sent < conn->wbuf.size()) {
        ssize_t n = ::write(conn->fd, &conn->wbuf[conn->wbuf_sent], conn->wbuf.size() - conn->wbuf_sent);
        if (n < 0) {
            if (errno == EAGAIN) {
                break;
            } else {
                perror("write");
                conn->state = STATE_END;
                break;
            }
        } else {
            conn->wbuf_sent += static_cast<size_t>(n);
        }
    }
    if (conn->wbuf_sent == conn->wbuf.size()) {
        conn->wbuf.clear();
        conn->wbuf_sent = 0;
        conn->state = STATE_REQ;
    }
}

// ===== Commands (same logic as your original handle_command) =====

#include <cstdlib>

void Server::handle_command(const std::vector<std::string>& cmd, std::string& out) {
    if (cmd.empty()) {
        out_error(out, "Empty command");
        return;
    }
    const std::string& command = cmd[0];

    if (command == "get") {
        if (cmd.size() != 2) { out_error(out, "Invalid number of arguments for 'get'"); return; }
        Entry* entry = db_.get(cmd[1]);
        if (entry && entry->type == Entry::STRING) out_string(out, entry->str_value);
        else out_nil(out);

    } else if (command == "set") {
        if (cmd.size() != 3) { out_error(out, "Invalid number of arguments for 'set'"); return; }
        Entry* entry = db_.get(cmd[1]);
        if (entry) {
            if (entry->type != Entry::STRING) { out_error(out, "Wrong type"); return; }
            entry->str_value = cmd[2];
        } else {
            entry = new Entry(cmd[1], cmd[2]);
            db_.put(cmd[1], entry);
        }
        out_ok(out);

    } else if (command == "del") {
        if (cmd.size() != 2) { out_error(out, "Invalid number of arguments for 'del'"); return; }
        db_.remove(cmd[1]);
        out_int(out, 1);

    } else if (command == "keys") {
        if (cmd.size() != 1) { out_error(out, "Invalid number of arguments for 'keys'"); return; }
        std::vector<std::string> keys = db_.keys();
        out_array(out, keys);

    } else if (command == "zadd") {
        if (cmd.size() != 4) { out_error(out, "Invalid number of arguments for 'zadd'"); return; }
        const std::string& key = cmd[1];
        double score = std::stod(cmd[2]);
        const std::string& member = cmd[3];

        Entry* entry = db_.get(key);
        if (!entry) {
            entry = new Entry(key, "");
            entry->type = Entry::ZSET;
            entry->zset_value = new ZSet();
            db_.put(key, entry);
        }
        if (entry->type != Entry::ZSET) { out_error(out, "Wrong type"); return; }
        bool added = entry->zset_value->zadd(member, score);
        out_int(out, added ? 1 : 0);

    } else if (command == "zrem") {
        if (cmd.size() != 3) { out_error(out, "Invalid number of arguments for 'zrem'"); return; }
        const std::string& key = cmd[1];
        const std::string& member = cmd[2];

        Entry* entry = db_.get(key);
        if (!entry || entry->type != Entry::ZSET) { out_error(out, "Wrong type or key does not exist"); return; }
        bool removed = entry->zset_value->zrem(member);
        out_int(out, removed ? 1 : 0);

    } else if (command == "zscore") {
        if (cmd.size() != 3) { out_error(out, "Invalid number of arguments for 'zscore'"); return; }
        const std::string& key = cmd[1];
        const std::string& member = cmd[2];

        Entry* entry = db_.get(key);
        if (!entry || entry->type != Entry::ZSET) { out_error(out, "Wrong type or key does not exist"); return; }
        double score;
        if (entry->zset_value->zscore(member, score)) out_double(out, score);
        else out_nil(out);

    } else if (command == "zquery") {
        if (cmd.size() != 6) { out_error(out, "Invalid number of arguments for 'zquery'"); return; }
        const std::string& key = cmd[1];
        double min_score = std::stod(cmd[2]);
        const std::string& min_member = cmd[3];
        int offset = std::stoi(cmd[4]);
        int limit = std::stoi(cmd[5]);

        Entry* entry = db_.get(key);
        if (!entry || entry->type != Entry::ZSET) { out_error(out, "Wrong type or key does not exist"); return; }
        std::vector<std::pair<std::string, double>> result = entry->zset_value->zquery(min_score, min_member, offset, limit);

        out.push_back(SER_ARR);
        uint32_t len = static_cast<uint32_t>(result.size() * 2);
        out.append((char*)&len, 4);
        for (const auto& pair : result) {
            out_string(out, pair.first);
            out_double(out, pair.second);
        }

    } else {
        out_error(out, "Unknown command");
    }
}
