#include "client/protocol.h"
#include "client/util.h"
#include <cstring>
#include <iostream>

void serialize_request(const std::vector<std::string>& cmd, std::vector<uint8_t>& out) {
    uint32_t argc = static_cast<uint32_t>(cmd.size());
    uint32_t data_len = 4; // length of argc
    for (const std::string& arg : cmd) {
        data_len += 4; // length of arg_len
        data_len += static_cast<uint32_t>(arg.size());
    }
    uint32_t total_len = data_len;

    out.resize(4 + data_len); // 4 bytes for len, plus data_len bytes for data
    uint32_t offset = 0;

    std::memcpy(&out[offset], &total_len, 4);
    offset += 4;

    std::memcpy(&out[offset], &argc, 4);
    offset += 4;

    for (const std::string& arg : cmd) {
        uint32_t arg_len = static_cast<uint32_t>(arg.size());
        std::memcpy(&out[offset], &arg_len, 4);
        offset += 4;
        std::memcpy(&out[offset], arg.data(), arg_len);
        offset += arg_len;
    }
}

// helper so both overloads share identical logic
static void deserialize_impl(const std::vector<uint8_t>& in, size_t& offset,
                             std::ostream& out, std::ostream& err) {
    if (offset >= in.size()) {
        die("Response parsing error: offset out of bounds");
    }
    uint8_t type = in[offset++];
    switch (type) {
        case SER_STR: {
            if (offset + 4 > in.size()) die("Response parsing error: string length");
            uint32_t len = 0;
            std::memcpy(&len, &in[offset], 4);
            offset += 4;
            if (offset + len > in.size()) die("Response parsing error: string data");
            std::string str(reinterpret_cast<const char*>(&in[offset]), len);
            offset += len;
            out << str << std::endl;
            break;
        }
        case SER_INT: {
            if (offset + 8 > in.size()) die("Response parsing error: int data");
            int64_t val = 0;
            std::memcpy(&val, &in[offset], 8);
            offset += 8;
            out << val << std::endl;
            break;
        }
        case SER_DBL: {
            if (offset + 8 > in.size()) die("Response parsing error: double data");
            double val = 0;
            std::memcpy(&val, &in[offset], 8);
            offset += 8;
            out << val << std::endl;
            break;
        }
        case SER_NIL: {
            out << "(nil)" << std::endl;
            break;
        }
        case SER_ERR: {
            if (offset + 4 > in.size()) die("Response parsing error: error length");
            uint32_t len = 0;
            std::memcpy(&len, &in[offset], 4);
            offset += 4;
            if (offset + len > in.size()) die("Response parsing error: error data");
            std::string str(reinterpret_cast<const char*>(&in[offset]), len);
            offset += len;
            err << "(error) " << str << std::endl;
            break;
        }
        case SER_ARR: {
            if (offset + 4 > in.size()) die("Response parsing error: array length");
            uint32_t len = 0;
            std::memcpy(&len, &in[offset], 4);
            offset += 4;
            for (uint32_t i = 0; i < len; ++i) {
                deserialize_impl(in, offset, out, err);
            }
            break;
        }
        default:
            die("Response parsing error: unknown type");
    }
}

void deserialize_response(const std::vector<uint8_t>& in, size_t& offset) {
    deserialize_impl(in, offset, std::cout, std::cerr);
}

void deserialize_response(const std::vector<uint8_t>& in, size_t& offset,
                          std::ostream& out, std::ostream& err) {
    deserialize_impl(in, offset, out, err);
}
