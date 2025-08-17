#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <iosfwd>
#include "common/serialization.h"

// Moved from client.cpp. Same signatures/logic so you can unit test them.
void serialize_request(const std::vector<std::string>& cmd, std::vector<uint8_t>& out);

// Original function wrote to std::cout/std::cerr directly.
// Keep it, but also provide an overload that takes output streams (handy for tests).
void deserialize_response(const std::vector<uint8_t>& in, size_t& offset);
void deserialize_response(const std::vector<uint8_t>& in, size_t& offset,
                          std::ostream& out, std::ostream& err);
