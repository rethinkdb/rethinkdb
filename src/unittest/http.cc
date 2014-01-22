// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <ctime>

#include "unittest/gtest.hpp"
#include "http/http.hpp"

namespace unittest {

TEST(Http, FormatTime) {

    std::string formatted_time = http_format_date(1356998463);

    EXPECT_EQ("Tue, 01 Jan 2013 00:01:03 GMT", formatted_time);

}

// Helper functions to create dummy objects
http_req_t http_req_encoding(const std::string &encoding) {
    http_req_t req;
    header_line_t accept_encoding;
    accept_encoding.key = "Accept-Encoding";
    accept_encoding.val = encoding;
    req.header_lines.push_back(accept_encoding);
    return req;
}

void test_encoding(const std::string &encoding, bool expected) {
    // Use at least 2k so compression has something to work with
    static std::string body;
    for (size_t i = 0; i < 2048; ++i) {
        body += 'a' + (i % 26);
    }

    http_req_t req = http_req_encoding(encoding);
    http_res_t res(HTTP_OK);
    res.set_body("application/text", body);

    EXPECT_EQ(expected, maybe_gzip_response(req, &res)) << "Incorrect handling of encoding '" + encoding + "'";
}

TEST(Http, Encodings) {
    // These encodings are valid and should result in compression
    test_encoding("gzip", true);
    test_encoding("gzip", true);
    test_encoding("gzip ;q=001", true);
    test_encoding("gzip;q =0.1", true);
    test_encoding("gzip  ;q= 1000.11111  ", true);
    test_encoding("  identity;q=1000.1111  ,  gzip;q=1000.11111", true);
    test_encoding("compress, gzip", true);
    test_encoding("*", true);
    test_encoding("compress;q=0.5, gzip;q=1.0", true);
    test_encoding("gzip;q=1.0, identity; q=0.5, *;q=0", true);
    test_encoding("geewhiz;q=1.0,geezip;q=2.0,*;q=0.5", true);
    test_encoding("geewhiz;q=0.0,geezip;q=2.2,*;q=3", true);

    // These encodings are valid but should not result in compression
    test_encoding("", false);
    test_encoding("*;q=0.0", false);
    test_encoding("gzip;q=0.1,identity;q=0.2", false);
    test_encoding("gzip;q=1,*;q=1.84", false);
    test_encoding("identity;q=1,*;q=0.5", false);
    test_encoding("geewhiz", false);
    test_encoding("geewhiz;q=1.0", false);
    test_encoding("geewhiz;q=1.0,geezip;q=2.0", false);

    // These encodings have bad syntax and should fail
    test_encoding("gzip:q=0.1", false);
    test_encoding("gzip;q=0 .1", false);
    test_encoding("gzip;q=0. 1", false);
    test_encoding("gzip:q=0x1", false);
    test_encoding("gzip;q=", false);
    test_encoding("gzip;q", false);
    test_encoding("gzip;=0.5", false);
    test_encoding("gzip=9.9", false);
    test_encoding("gzip;", false);
    test_encoding("gzip;s=0.6", false);
    test_encoding("gzip;deflate", false);
    test_encoding("gzip,deflate,*=0.0", false);
    test_encoding("g^zip", false);
    test_encoding("g_zip", false);
}

}
