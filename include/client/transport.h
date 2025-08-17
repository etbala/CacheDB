#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

// Thin transport interface so logic can be tested with a fake transport in gtest.
struct ITransport {
    virtual ~ITransport() = default;
    virtual void connect(const std::string& host, uint16_t port) = 0;
    virtual void send_all(const uint8_t* data, size_t len) = 0;
    virtual void recv_all(uint8_t* data, size_t len) = 0;
    virtual void close() = 0;
};

// Concrete TCP transport used by main program.
class TcpTransport : public ITransport {
public:
    TcpTransport();
    ~TcpTransport() override;

    void connect(const std::string& host, uint16_t port) override;
    void send_all(const uint8_t* data, size_t len) override;
    void recv_all(uint8_t* data, size_t len) override;
    void close() override;

private:
    int sockfd_;
};
