#include "server/protocol.h"
#include <cstring>

void out_string(std::string& out, const std::string& str) {
    out.push_back(SER_STR);
    uint32_t len = static_cast<uint32_t>(str.size());
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
    uint32_t len = static_cast<uint32_t>(msg.size());
    out.append((char*)&len, 4);
    out.append(msg);
}

void out_ok(std::string& out) {
    out_string(out, "OK");
}

void out_array(std::string& out, const std::vector<std::string>& arr) {
    out.push_back(SER_ARR);
    uint32_t len = static_cast<uint32_t>(arr.size());
    out.append((char*)&len, 4);
    for (const std::string& s : arr) {
        out_string(out, s);
    }
}

void out_double(std::string& out, double val) {
    out.push_back(SER_DBL);
    out.append((char*)&val, 8);
}

// Parsing request from client (unchanged)
int parse_request(const uint8_t* data, size_t len, std::vector<std::string>& out) {
    if (len < 4) return -1;
    uint32_t argc = 0;
    std::memcpy(&argc, data, 4);
    if (argc > 1024) return -1;

    size_t pos = 4;
    for (uint32_t i = 0; i < argc; ++i) {
        if (pos + 4 > len) return -1;
        uint32_t arg_len = 0;
        std::memcpy(&arg_len, data + pos, 4);
        pos += 4;
        if (pos + arg_len > len) return -1;
        out.emplace_back((char*)data + pos, arg_len);
        pos += arg_len;
    }
    if (pos != len) return -1;
    return 0;
}
