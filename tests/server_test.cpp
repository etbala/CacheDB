#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include "server/server.h"
#include "server/protocol.h"

// -------- Test-only decoder for the server's serialization format --------
enum TKind { T_STR, T_NIL, T_INT, T_ERR, T_ARR, T_DBL };

struct TVal {
    TKind kind;
    std::string s;
    int64_t i = 0;
    double d = 0.0;
    std::vector<TVal> arr;
};

static bool parseOne(const std::string& buf, size_t& pos, TVal& out) {
    if (pos >= buf.size()) return false;
    uint8_t tag = static_cast<uint8_t>(buf[pos++]);
    switch (tag) {
        case SER_STR: {
            if (pos + 4 > buf.size()) return false;
            uint32_t len = 0; std::memcpy(&len, &buf[pos], 4); pos += 4;
            if (pos + len > buf.size()) return false;
            out.kind = T_STR; out.s.assign(&buf[pos], &buf[pos + len]); pos += len;
            return true;
        }
        case SER_NIL:
            out.kind = T_NIL; return true;
        case SER_INT: {
            if (pos + 8 > buf.size()) return false;
            int64_t v = 0; std::memcpy(&v, &buf[pos], 8); pos += 8;
            out.kind = T_INT; out.i = v; return true;
        }
        case SER_ERR: {
            if (pos + 4 > buf.size()) return false;
            uint32_t len = 0; std::memcpy(&len, &buf[pos], 4); pos += 4;
            if (pos + len > buf.size()) return false;
            out.kind = T_ERR; out.s.assign(&buf[pos], &buf[pos + len]); pos += len;
            return true;
        }
        case SER_ARR: {
            if (pos + 4 > buf.size()) return false;
            uint32_t n = 0; std::memcpy(&n, &buf[pos], 4); pos += 4;
            out.kind = T_ARR; out.arr.clear(); out.arr.reserve(n);
            for (uint32_t k = 0; k < n; ++k) {
                TVal elem;
                if (!parseOne(buf, pos, elem)) return false;
                out.arr.push_back(std::move(elem));
            }
            return true;
        }
        case SER_DBL: {
            if (pos + 8 > buf.size()) return false;
            double v = 0; std::memcpy(&v, &buf[pos], 8); pos += 8;
            out.kind = T_DBL; out.d = v; return true;
        }
        default:
            return false;
    }
}

static TVal decode(const std::string& buf) {
    size_t pos = 0;
    TVal v{};
    bool ok = parseOne(buf, pos, v);
    if (!ok || pos != buf.size()) {
        // For tests, fail hard with a clear message.
        ADD_FAILURE() << "Failed to decode response buffer (size=" << buf.size() << ", pos=" << pos << ")";
    }
    return v;
}

// ------------------------------ Tests ------------------------------

TEST(ServerCommands, EmptyCommandErrors) {
    Server s;
    std::string out;
    s.handle_command({}, out);
    TVal v = decode(out);
    ASSERT_EQ(v.kind, T_ERR);
    EXPECT_NE(v.s.find("Empty command"), std::string::npos);
}

TEST(ServerCommands, UnknownCommandErrors) {
    Server s;
    std::string out;
    s.handle_command({"wtf"}, out);
    TVal v = decode(out);
    ASSERT_EQ(v.kind, T_ERR);
    EXPECT_NE(v.s.find("Unknown command"), std::string::npos);
}

TEST(ServerCommands, SetGetRoundTrip) {
    Server s;
    std::string out;

    s.handle_command({"set", "k1", "v1"}, out);
    auto ok = decode(out);
    ASSERT_EQ(ok.kind, T_STR);  // out_ok => "OK"
    EXPECT_EQ(ok.s, "OK");

    out.clear();
    s.handle_command({"get", "k1"}, out);
    auto got = decode(out);
    ASSERT_EQ(got.kind, T_STR);
    EXPECT_EQ(got.s, "v1");
}

TEST(ServerCommands, GetMissingIsNil) {
    Server s;
    std::string out;
    s.handle_command({"get", "nope"}, out);
    auto v = decode(out);
    ASSERT_EQ(v.kind, T_NIL);
}

TEST(ServerCommands, DelAlwaysReturnsOne) {
    Server s;
    std::string out;

    // Delete nonexistent
    s.handle_command({"del", "missing"}, out);
    auto v1 = decode(out);
    ASSERT_EQ(v1.kind, T_INT);
    EXPECT_EQ(v1.i, 1);

    out.clear();
    // Put something then delete
    s.handle_command({"set", "todel", "x"}, out);
    out.clear();
    s.handle_command({"del", "todel"}, out);
    auto v2 = decode(out);
    ASSERT_EQ(v2.kind, T_INT);
    EXPECT_EQ(v2.i, 1);
}

TEST(ServerCommands, KeysListsAllKeys) {
    Server s;
    std::string out;

    s.handle_command({"set", "a", "1"}, out); out.clear();
    s.handle_command({"set", "b", "2"}, out); out.clear();
    s.handle_command({"set", "c", "3"}, out); out.clear();

    s.handle_command({"keys"}, out);
    auto v = decode(out);
    ASSERT_EQ(v.kind, T_ARR);

    std::vector<std::string> got;
    for (auto& e : v.arr) {
        ASSERT_EQ(e.kind, T_STR);
        got.push_back(e.s);
    }
    // Order is unspecified, so check membership.
    auto has = [&](const char* k){ return std::find(got.begin(), got.end(), k) != got.end(); };
    EXPECT_TRUE(has("a"));
    EXPECT_TRUE(has("b"));
    EXPECT_TRUE(has("c"));
}

TEST(ServerCommands, WrongTypeErrors) {
    Server s;
    std::string out;

    // Create a ZSET key
    s.handle_command({"zadd", "myz", "1.5", "m"}, out); out.clear();

    // set on ZSET -> error
    s.handle_command({"set", "myz", "xxx"}, out);
    auto e1 = decode(out);
    ASSERT_EQ(e1.kind, T_ERR);
    EXPECT_NE(e1.s.find("Wrong type"), std::string::npos);

    out.clear();
    // zadd on STRING -> error
    s.handle_command({"set", "skey", "str"}, out); out.clear();
    s.handle_command({"zadd", "skey", "2.0", "m"}, out);
    auto e2 = decode(out);
    ASSERT_EQ(e2.kind, T_ERR);
    EXPECT_NE(e2.s.find("Wrong type"), std::string::npos);

    out.clear();
    // zscore on STRING -> error
    s.handle_command({"zscore", "skey", "m"}, out);
    auto e3 = decode(out);
    ASSERT_EQ(e3.kind, T_ERR);
    EXPECT_NE(e3.s.find("Wrong type"), std::string::npos);
}

TEST(ServerCommands, ZAddZScoreZRem) {
    Server s;
    std::string out;

    // zadd returns int (implementation always 1)
    s.handle_command({"zadd", "myz", "10.5", "alice"}, out);
    auto a1 = decode(out);
    ASSERT_EQ(a1.kind, T_INT);
    EXPECT_EQ(a1.i, 1);
    out.clear();

    // zscore -> 10.5
    s.handle_command({"zscore", "myz", "alice"}, out);
    auto sc1 = decode(out);
    ASSERT_EQ(sc1.kind, T_DBL);
    EXPECT_DOUBLE_EQ(sc1.d, 10.5);
    out.clear();

    // update score (still returns 1)
    s.handle_command({"zadd", "myz", "12.0", "alice"}, out);
    auto a2 = decode(out);
    ASSERT_EQ(a2.kind, T_INT);
    EXPECT_EQ(a2.i, 1);
    out.clear();

    s.handle_command({"zscore", "myz", "alice"}, out);
    auto sc2 = decode(out);
    ASSERT_EQ(sc2.kind, T_DBL);
    EXPECT_DOUBLE_EQ(sc2.d, 12.0);
    out.clear();

    // remove
    s.handle_command({"zrem", "myz", "alice"}, out);
    auto r1 = decode(out);
    ASSERT_EQ(r1.kind, T_INT);
    EXPECT_EQ(r1.i, 1);
    out.clear();

    // score now NIL
    s.handle_command({"zscore", "myz", "alice"}, out);
    auto scnil = decode(out);
    ASSERT_EQ(scnil.kind, T_NIL);
    out.clear();

    // removing again -> 0
    s.handle_command({"zrem", "myz", "alice"}, out);
    auto r0 = decode(out);
    ASSERT_EQ(r0.kind, T_INT);
    EXPECT_EQ(r0.i, 0);
}

TEST(ServerCommands, ZScoreMissingMemberIsNil) {
    Server s; std::string out;
    s.handle_command({"zadd", "myz", "1", "a"}, out); out.clear();
    s.handle_command({"zscore", "myz", "nope"}, out);
    auto v = decode(out);
    ASSERT_EQ(v.kind, T_NIL);
}

TEST(ServerCommands, ZQueryPagingAndOrdering) {
    Server s; std::string out;
    // Prepare sorted set with multiple members
    s.handle_command({"zadd", "myz", "1.0", "a"}, out); out.clear();
    s.handle_command({"zadd", "myz", "1.0", "b"}, out); out.clear();
    s.handle_command({"zadd", "myz", "2.0", "c"}, out); out.clear();
    s.handle_command({"zadd", "myz", "3.0", "d"}, out); out.clear();

    // Query from min_score=1.0, min_member="b", offset=0, limit=3
    s.handle_command({"zquery", "myz", "1.0", "b", "0", "3"}, out);
    auto q = decode(out);
    ASSERT_EQ(q.kind, T_ARR);
    // Response is [member, score, member, score, ...]
    ASSERT_EQ(q.arr.size(), 3u * 2u);
    ASSERT_EQ(q.arr[0].kind, T_STR); EXPECT_EQ(q.arr[0].s, "b");
    ASSERT_EQ(q.arr[1].kind, T_DBL); EXPECT_DOUBLE_EQ(q.arr[1].d, 1.0);
    ASSERT_EQ(q.arr[2].kind, T_STR); EXPECT_EQ(q.arr[2].s, "c");
    ASSERT_EQ(q.arr[3].kind, T_DBL); EXPECT_DOUBLE_EQ(q.arr[3].d, 2.0);
    ASSERT_EQ(q.arr[4].kind, T_STR); EXPECT_EQ(q.arr[4].s, "d");
    ASSERT_EQ(q.arr[5].kind, T_DBL); EXPECT_DOUBLE_EQ(q.arr[5].d, 3.0);

    out.clear();
    // With offset=1, limit=2 from same start: expect c, d
    s.handle_command({"zquery", "myz", "1.0", "b", "1", "2"}, out);
    auto q2 = decode(out);
    ASSERT_EQ(q2.kind, T_ARR);
    ASSERT_EQ(q2.arr.size(), 2u * 2u);
    EXPECT_EQ(q2.arr[0].s, "c");
    EXPECT_DOUBLE_EQ(q2.arr[1].d, 2.0);
    EXPECT_EQ(q2.arr[2].s, "d");
    EXPECT_DOUBLE_EQ(q2.arr[3].d, 3.0);
}

TEST(ServerCommands, ArityErrors) {
    Server s; std::string out;

    s.handle_command({"get"}, out);
    auto e1 = decode(out);
    ASSERT_EQ(e1.kind, T_ERR);
    EXPECT_NE(e1.s.find("Invalid number of arguments"), std::string::npos);
    out.clear();

    s.handle_command({"set", "konly"}, out);
    auto e2 = decode(out);
    ASSERT_EQ(e2.kind, T_ERR);
    EXPECT_NE(e2.s.find("Invalid number of arguments"), std::string::npos);
    out.clear();

    s.handle_command({"zadd", "myz", "1.0"}, out);
    auto e3 = decode(out);
    ASSERT_EQ(e3.kind, T_ERR);
    EXPECT_NE(e3.s.find("Invalid number of arguments"), std::string::npos);
}
