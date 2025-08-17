#include <gtest/gtest.h>
#include <sstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

#include "client/protocol.h"
#include "client/client_loop.h"
#include "client/transport.h"
#include "client/util.h"

// ---------- Test helpers (build reply frames and verify requests) ----------

namespace testutil {

// Build a single RESP-like body element (not framed) per your protocol.
inline std::vector<uint8_t> ser_str(const std::string& s) {
    std::vector<uint8_t> b;
    b.push_back(SER_STR);
    uint32_t len = static_cast<uint32_t>(s.size());
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&len);
    b.insert(b.end(), p, p+4);
    b.insert(b.end(), s.begin(), s.end());
    return b;
}

inline std::vector<uint8_t> ser_int(int64_t v) {
    std::vector<uint8_t> b;
    b.push_back(SER_INT);
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
    b.insert(b.end(), p, p+8);
    return b;
}

inline std::vector<uint8_t> ser_dbl(double v) {
    std::vector<uint8_t> b;
    b.push_back(SER_DBL);
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
    b.insert(b.end(), p, p+8);
    return b;
}

inline std::vector<uint8_t> ser_nil() {
    return std::vector<uint8_t>{ static_cast<uint8_t>(SER_NIL) };
}

inline std::vector<uint8_t> ser_err(const std::string& msg) {
    std::vector<uint8_t> b;
    b.push_back(SER_ERR);
    uint32_t len = static_cast<uint32_t>(msg.size());
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&len);
    b.insert(b.end(), p, p+4);
    b.insert(b.end(), msg.begin(), msg.end());
    return b;
}

inline std::vector<uint8_t> ser_arr(const std::vector<std::vector<uint8_t>>& elements) {
    std::vector<uint8_t> b;
    b.push_back(SER_ARR);
    uint32_t len = static_cast<uint32_t>(elements.size());
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&len);
    b.insert(b.end(), p, p+4);
    for (const auto& e : elements) {
        b.insert(b.end(), e.begin(), e.end());
    }
    return b;
}

// Frame a body with a 4-byte little-endian length prefix (as your client expects).
inline std::vector<uint8_t> frame(const std::vector<uint8_t>& body) {
    std::vector<uint8_t> out(4);
    uint32_t n = static_cast<uint32_t>(body.size());
    std::memcpy(out.data(), &n, 4);
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

// Convenience to build the expected request buffer for a given command vector.
inline std::vector<uint8_t> build_request(const std::vector<std::string>& cmd) {
    std::vector<uint8_t> r;
    serialize_request(cmd, r);
    return r;
}

} // namespace testutil

// ---------- Fake transport for REPL tests ----------

struct FakeTransport : ITransport {
    // What the "server" will send to the client (a sequence of framed replies).
    std::vector<uint8_t> scripted_recv;
    size_t recv_off = 0;

    // What the client sent to the server.
    std::vector<uint8_t> captured_send;

    void connect(const std::string&, uint16_t) override {}
    void send_all(const uint8_t* data, size_t len) override {
        captured_send.insert(captured_send.end(), data, data + len);
    }
    void recv_all(uint8_t* data, size_t len) override {
        if (recv_off + len > scripted_recv.size()) {
            // Simulate EOF/failure -> this will trigger die("recv failed"), so we just mimic short read
            // but tests that want death should ensure this.
            std::memset(data, 0, len);
            return;
        }
        std::memcpy(data, scripted_recv.data() + recv_off, len);
        recv_off += len;
    }
    void close() override {}
};

// ======================= Protocol: serialize tests ==========================

TEST(ProtocolSerialize, CorrectHeaderAndArgs) {
    std::vector<std::string> cmd = {"SET", "k", "v"};
    auto buf = testutil::build_request(cmd);

    ASSERT_GE(buf.size(), 4u);
    uint32_t total_len = 0;
    std::memcpy(&total_len, buf.data(), 4);
    EXPECT_EQ(total_len + 4, buf.size()); // first 4 bytes = payload size

    // argc at offset 4
    uint32_t argc = 0;
    std::memcpy(&argc, buf.data() + 4, 4);
    EXPECT_EQ(argc, 3u);

    // First arg length should start at offset 8.
    uint32_t arg0_len = 0;
    std::memcpy(&arg0_len, buf.data() + 8, 4);
    EXPECT_EQ(arg0_len, 3u); // "SET"
}

// ======================= Protocol: deserialize tests ========================

TEST(ProtocolDeserialize, StringIntDoubleNilError) {
    using namespace testutil;
    auto body = ser_arr({
        ser_str("hello"),
        ser_int(42),
        ser_dbl(3.5),
        ser_nil(),
        ser_err("boom"),
    });

    size_t off = 0;
    std::ostringstream out, err;
    deserialize_response(body, off, out, err);

    // Order matters and each prints with '\n'
    std::string expected =
        "hello\n"
        "42\n"
        "3.5\n"
        "(nil)\n";
    EXPECT_EQ(out.str(), expected);
    EXPECT_EQ(err.str(), "(error) boom\n");
    EXPECT_EQ(off, body.size());
}

TEST(ProtocolDeserialize, NestedArrays) {
    using namespace testutil;
    auto body = ser_arr({
        ser_str("outer"),
        ser_arr({
            ser_str("inner1"),
            ser_int(7),
        }),
        ser_str("tail"),
    });

    size_t off = 0;
    std::ostringstream out, err;
    deserialize_response(body, off, out, err);
    std::string expected =
        "outer\n"
        "inner1\n"
        "7\n"
        "tail\n";
    EXPECT_EQ(out.str(), expected);
    EXPECT_TRUE(err.str().empty());
}

TEST(ProtocolDeserializeDeath, BadLengthDies) {
    using namespace testutil;
    // Construct a string with declared length bigger than buffer to trigger die()
    std::vector<uint8_t> body;
    body.push_back(SER_STR);
    uint32_t len = 1000; // lie
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&len);
    body.insert(body.end(), p, p+4);
    body.push_back('x'); // only 1 byte of actual data

    size_t off = 0;
    std::ostringstream out, err;
    // die() calls std::exit, so use a death test.
    EXPECT_DEATH(deserialize_response(body, off, out, err),
                 "Response parsing error: string data");
}

// ======================= REPL tests via FakeTransport =======================

TEST(ClientRepl, SingleCommandRoundTrip) {
    using namespace testutil;

    // Simulate server replying "PONG" after "PING"
    auto reply_body = ser_str("PONG");
    auto framed = frame(reply_body);

    FakeTransport t;
    t.scripted_recv = framed; // Repl will read header(4), then body

    std::istringstream in("PING\n");  // user enters one line
    std::ostringstream out, err;

    // The REPL prints a prompt before reading and again after handling the line.
    int rc = run_client_repl(t, in, out, err);
    EXPECT_EQ(rc, 0);

    // Out should contain prompt, then server output, then prompt (blocked by EOF so not printed again)
    // Our loop prints prompt before each getline; after last iteration (EOF), it exits without printing a final prompt.
    EXPECT_EQ(out.str(), "> PONG\n> ");

    // Verify the request that got sent matches protocol encoding for ["PING"]
    auto expected_req = build_request({"PING"});
    EXPECT_EQ(t.captured_send, expected_req);
    EXPECT_TRUE(err.str().empty());
}

TEST(ClientRepl, MultipleCommandsAndEmptyLines) {
    using namespace testutil;

    // Server replies 3 lines for three non-empty inputs; empty lines are ignored by client.
    std::vector<uint8_t> script;
    auto a = frame(ser_int(1));
    auto b = frame(ser_str("two"));
    auto c = frame(ser_nil());
    script.insert(script.end(), a.begin(), a.end());
    script.insert(script.end(), b.begin(), b.end());
    script.insert(script.end(), c.begin(), c.end());

    FakeTransport t;
    t.scripted_recv = script;

    std::istringstream in("\nINCR\nECHO two\nNULL\n");
    std::ostringstream out, err;

    int rc = run_client_repl(t, in, out, err);
    EXPECT_EQ(rc, 0);

    // One prompt per getline; empty first line prints a prompt but no output; then 3 outputs.
    std::string expected_out =
        "> "      // before empty line
        "> "      // before INCR
        "1\n"
        "> "      // before ECHO two
        "two\n"
        "> "      // before NULL
        "(nil)\n"
        "> ";     // after last line (EOF, so no more output)
    EXPECT_EQ(out.str(), expected_out);
    EXPECT_TRUE(err.str().empty());

    // Verify three requests were sent in order
    auto r1 = build_request({"INCR"});
    auto r2 = build_request({"ECHO", "two"});
    auto r3 = build_request({"NULL"});

    // Concatenate expected requests; REPL sends each back-to-back.
    std::vector<uint8_t> expected;
    expected.insert(expected.end(), r1.begin(), r1.end());
    expected.insert(expected.end(), r2.begin(), r2.end());
    expected.insert(expected.end(), r3.begin(), r3.end());
    EXPECT_EQ(t.captured_send, expected);
}

TEST(ClientReplDeath, ResponseTooLarge) {
    FakeTransport t;

    // Header says > 10MB which should trigger die("Response too large")
    uint32_t huge = 10 * 1024 * 1024 + 1;
    std::vector<uint8_t> framed(4);
    std::memcpy(framed.data(), &huge, 4);
    t.scripted_recv = framed;

    std::istringstream in("ANY\n");
    std::ostringstream out, err;

    EXPECT_DEATH(
        {
            run_client_repl(t, in, out, err);
        },
        "Response too large"
    );
}

// ======================= Integration-ish serialization check =================

TEST(ProtocolSerialize, ArgumentsMarshaledCorrectly) {
    std::vector<std::string> cmd = {"MSET", "key", "value with space", "123"};
    std::vector<uint8_t> buf;
    serialize_request(cmd, buf);

    // Read argc
    uint32_t argc = 0;
    std::memcpy(&argc, buf.data() + 4, 4);
    ASSERT_EQ(argc, 4u);

    size_t off = 8;
    auto read_len = [&](uint32_t& L) {
        std::memcpy(&L, buf.data() + off, 4);
        off += 4;
    };
    auto read_bytes = [&](std::string& s, uint32_t L) {
        s.assign(reinterpret_cast<const char*>(buf.data() + off), L);
        off += L;
    };

    uint32_t L0; read_len(L0);
    std::string a0; read_bytes(a0, L0);
    EXPECT_EQ(a0, "MSET");

    uint32_t L1; read_len(L1);
    std::string a1; read_bytes(a1, L1);
    EXPECT_EQ(a1, "key");

    uint32_t L2; read_len(L2);
    std::string a2; read_bytes(a2, L2);
    EXPECT_EQ(a2, "value with space");

    uint32_t L3; read_len(L3);
    std::string a3; read_bytes(a3, L3);
    EXPECT_EQ(a3, "123");

    // Buffer should end exactly here
    EXPECT_EQ(off, buf.size());
}

