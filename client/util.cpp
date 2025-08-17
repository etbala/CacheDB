#include "client/util.h"
#include <cerrno>
#include <cstdlib>
#include <iostream>

[[noreturn]] void die(const std::string& message) {
    int err = errno;
    std::cerr << "[" << err << "] " << message << std::endl;
    std::exit(EXIT_FAILURE);
}
