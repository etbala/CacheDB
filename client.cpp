#include "client/client_loop.h"
#include "client/transport.h"
#include <iostream>

int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    uint16_t port = 1234;

    // (Optional) allow overrides: client 127.0.0.1 1234
    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = static_cast<uint16_t>(std::stoi(argv[2]));

    TcpTransport transport;
    transport.connect(host, port);

    // Use std::cin/std::cout/std::cerr in production; in tests you'll pass string streams.
    return run_client_repl(transport, std::cin, std::cout, std::cerr);
}
