#include "client/client_loop.h"
#include "client/protocol.h"
#include "client/util.h"

#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

int run_client_repl(ITransport& transport,
                    std::istream& in,
                    std::ostream& out,
                    std::ostream& err) {

    std::string line;
    while (true) {
        out << "> " << std::flush;
        if (!std::getline(in, line)) break;

        std::istringstream iss(line);
        std::vector<std::string> cmd;
        std::string arg;
        while (iss >> arg) cmd.push_back(arg);
        if (cmd.empty()) continue;

        // build request (unchanged logic)
        std::vector<uint8_t> request;
        serialize_request(cmd, request);

        // send request
        transport.send_all(request.data(), request.size());

        // header (4 bytes length)
        uint8_t header[4];
        transport.recv_all(header, 4);
        uint32_t resp_len = 0;
        std::memcpy(&resp_len, header, 4);
        if (resp_len > 10 * 1024 * 1024) die("Response too large");

        // body
        std::vector<uint8_t> body(resp_len);
        transport.recv_all(body.data(), resp_len);

        // parse & print
        size_t offset = 0;
        deserialize_response(body, offset, out, err);
    }

    return 0;
}
