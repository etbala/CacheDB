#pragma once
#include <string>

// Same die() you had, made reusable.
[[noreturn]] void die(const std::string& message);
