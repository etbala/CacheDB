#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "common/serialization.h"

// Serialization helpers (same behavior/signatures as before)
void out_string(std::string& out, const std::string& str);
void out_nil(std::string& out);
void out_int(std::string& out, int64_t val);
void out_error(std::string& out, const std::string& msg);
void out_ok(std::string& out);
void out_array(std::string& out, const std::vector<std::string>& arr);
void out_double(std::string& out, double val);

// Request parsing (same behavior as before)
int parse_request(const uint8_t* data, size_t len, std::vector<std::string>& out);
