
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <iostream>
#include <string>
#include <vector>

#include "socket.h"
#include "poll_manager.h"

static void msg(const std::string& message) {
    std::cerr << message << std::endl;
}

static void die(const std::string& message) {
    int err = errno;
    std::cerr << "[" << err << "] " << message << std::endl;
    std::abort();
}

// enum {
//     STATE_REQ = 0,
//     STATE_RES = 1,
//     STATE_END = 2,  // mark the connection for deletion
// };

// struct Conn {
//     int fd = -1;
//     uint32_t state = 0;     // either STATE_REQ or STATE_RES
//     // buffer for reading
//     size_t rbuf_size = 0;
//     uint8_t rbuf[4 + k_max_msg];
//     // buffer for writing
//     size_t wbuf_size = 0;
//     size_t wbuf_sent = 0;
//     uint8_t wbuf[4 + k_max_msg];
//     uint64_t idle_start = 0;
//     // timer
//     DList idle_list;
// };

// // global variables
// static struct {
//     HMap db;
//     // a map of all client connections, keyed by fd
//     std::vector<Conn *> fd2conn;
//     // timers for idle connections
//     DList idle_list;
//     // timers for TTLs
//     std::vector<HeapItem> heap;
//     // the thread pool
//     TheadPool tp;
// } g_data;

int main() {
    NetworkSocket socket;
    if (!socket.okay()) {
        die("socket()");
    }

    socket.set_sock_option(SOL_SOCKET, SO_REUSEADDR, 1);
    socket.bind(1234);
    socket.listen();
    socket.set_non_blocking(true);

    std::vector<struct pollfd> poll_args;
    while (true) {
        // prepare the arguments of the poll()
        poll_args.clear();
        // for convenience, the listening fd is put in the first position
        struct pollfd pfd = { socket.get_fd(), POLLIN, 0 };
        poll_args.push_back(pfd);

        // connection fds
        // Placeholder: Iterate through your connections and add them to poll_args
        for (Conn *conn : g_data.fd2conn) {
            if (!conn) {
                continue;
            }
            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events = pfd.events | POLLERR;
            poll_args.push_back(pfd);
        }

        // poll for active fds
        int timeout_ms = 1000;
        // int timeout_ms = next_timer_ms();
        int rv = socket.poll(poll_args, timeout_ms);
        if (rv < 0) {
            die("poll");
        }

        // process active connections
        for (size_t i = 1; i < poll_args.size(); ++i) {
            if (poll_args[i].revents) {
                Conn *conn = g_data.fd2conn[poll_args[i].fd];
                connection_io(conn);
                if (conn->state == STATE_END) {
                    // client closed normally, or something bad happened.
                    // destroy this connection
                    conn_done(conn);
                }
            }
        }

        // handle timers
        process_timers();

        // try to accept a new connection if the listening fd is active
        if (poll_args[0].revents) {
            accept_new_conn(socket.get_fd());
        }
    }

    return 0;

    // PollManager poll_manager;
    // poll_manager.add_socket(socket, POLLIN);

    // // the event loop
    // while (true) {
    //     // Reset poll_fds before each poll
    //     poll_manager.get_poll_fds().clear();
    //     poll_manager.add_socket(socket, POLLIN);

    //     // Placeholder: Iterate through your connections and add them to poll_manager

    //     int timeout_ms = next_timer_ms();
    //     int rv = poll_manager.poll(timeout_ms);
    //     if (rv < 0) {
    //         die("poll");
    //     }

    //     auto& poll_fds = poll_manager.get_poll_fds();
    //     for (size_t i = 1; i < poll_fds.size(); ++i) {
    //         if (poll_fds[i].revents) {
    //             // Placeholder: Retrieve the corresponding connection and process it
    //             // connection_io(conn);
    //             // if (conn->state == STATE_END) {
    //             //     conn_done(conn);
    //             // }
    //         }
    //     }

    //     process_timers();

    //     if (poll_fds[0].revents) {
    //         accept_new_conn(socket.get_fd());
    //     }
    // }

    // return 0;
}