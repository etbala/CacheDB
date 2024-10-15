#include <iostream>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <stdexcept>

void die(const std::string& message) {
    int err = errno;
    std::cerr << "[" << err << "] " << message << std::endl;
    std::abort();
}

int main() {
    // try {
    //     NetworkSocket socket;
    //     socket.connect("127.0.0.1", 1234);

    //     socket.write("hello");

    //     std::string response = socket.read(64);
    //     std::cout << "server says: " << response << std::endl;
    // } catch (const std::runtime_error& e) {
    //     die(e.what());
    // }

    return 0;
}