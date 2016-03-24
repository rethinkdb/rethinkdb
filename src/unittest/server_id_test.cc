// Copyright 2010-2016 RethinkDB, all rights reserved.

#include <set>

#include "containers/archive/string_stream.hpp"
#include "rpc/connectivity/server_id.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

TEST(ServerIdTest, Generate) {
    std::set<server_id_t> ids;
    for (int i = 0; i < 1000; ++i) {
        server_id_t sid = server_id_t::generate_server_id();
        ASSERT_FALSE(sid.is_proxy());
        ids.insert(sid);
        server_id_t pid = server_id_t::generate_proxy_id();
        ASSERT_TRUE(pid.is_proxy());
        ids.insert(pid);
    }
    ASSERT_EQ(ids.size(), 2000);
}

TEST(ServerIdTest, PrintAndParse) {
    for (int i = 0; i < 1000; ++i) {
        server_id_t sid = server_id_t::generate_server_id();
        server_id_t parsed_sid;
        ASSERT_TRUE(str_to_server_id(sid.print(), &parsed_sid));
        ASSERT_EQ(sid, parsed_sid);

        server_id_t pid = server_id_t::generate_proxy_id();;
        server_id_t parsed_pid;
        ASSERT_TRUE(str_to_server_id(pid.print(), &parsed_pid));
        ASSERT_EQ(pid, parsed_pid);
    }
}

server_id_t roundtrip_serialize(const server_id_t &sid) {
    write_message_t wm;
    string_stream_t write_stream;
    serialize_universal(&wm, sid);
    int send_res = send_write_message(&write_stream, &wm);
    EXPECT_EQ(0, send_res);

    std::string serialized_value = write_stream.str();
    string_read_stream_t read_stream(std::move(serialized_value), 0);

    server_id_t res;
    archive_result_t des_res = deserialize_universal(&read_stream, &res);
    EXPECT_EQ(archive_result_t::SUCCESS, des_res);
    return res;
}

TEST(ServerIdTest, Serialization) {
    for (int i = 0; i < 1000; ++i) {
        server_id_t sid = server_id_t::generate_server_id();
        ASSERT_EQ(sid, roundtrip_serialize(sid));

        server_id_t pid = server_id_t::generate_proxy_id();;
        ASSERT_EQ(pid, roundtrip_serialize(pid));
    }
}

TEST(ServerIdTest, NilId) {
    server_id_t sid = server_id_t::from_server_uuid(nil_uuid());
    ASSERT_EQ(sid, roundtrip_serialize(sid));

    server_id_t pid = server_id_t::from_proxy_uuid(nil_uuid());;
    ASSERT_EQ(pid, roundtrip_serialize(pid));
}

}  // namespace unittest

