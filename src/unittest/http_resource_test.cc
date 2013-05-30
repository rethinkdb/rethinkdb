#include "unittest/gtest.hpp"

#include "http/http.hpp"

namespace unittest {

void RunThrowingResourceTest(const char *path) {
    bool caught_exception = false;
    try {
        http_req_t::resource_t resource(path);
        auto it = resource.begin();
        auto jt = resource.end();
    } catch (const std::invalid_argument &ex) {
        caught_exception = true;
    }

    ASSERT_TRUE(caught_exception);
}

TEST(HttpResourceTest, EmptyPath) {
    RunThrowingResourceTest("");
}

TEST(HttpResourceTest, DoesNotStartWithSlash) {
    RunThrowingResourceTest("foo/");
}

TEST(HttpResourceTest, Slash) {
    http_req_t::resource_t resource("/");
    auto it = resource.begin();
    auto jt = resource.end();
    ASSERT_EQ(it, jt);
}

TEST(HttpResourceTest, OneElt) {
    http_req_t::resource_t resource("/foo");
    auto it = resource.begin();
    auto jt = resource.end();
    ASSERT_EQ("foo", *it);
    ++it;
    ASSERT_EQ(jt, it);
}

TEST(HttpResourceTest, OneEltSlash) {
    http_req_t::resource_t resource("/foo/");
    auto it = resource.begin();
    auto jt = resource.end();
    ASSERT_EQ("foo", *it);
    ++it;
    ASSERT_NE(jt, it);
    ASSERT_EQ("", *it);
    ++it;
    ASSERT_EQ(jt, it);
}

TEST(HttpResourceTest, TwoElt) {
    http_req_t::resource_t resource("/foo/bar");
    auto it = resource.begin();
    auto jt = resource.end();
    ASSERT_EQ("foo", *it);
    ++it;
    ASSERT_EQ("bar", *it);
    ++it;
    ASSERT_EQ(jt, it);
}

TEST(HttpResourceTest, TwoEltSlash) {
    http_req_t::resource_t resource("/foo/bar/");
    auto it = resource.begin();
    auto jt = resource.end();
    ASSERT_EQ("foo", *it);
    ++it;
    ASSERT_EQ("bar", *it);
    ++it;
    ASSERT_NE(jt, it);
    ASSERT_EQ("", *it);
    ++it;
    ASSERT_EQ(jt, it);
}

TEST(HttpResourceTest, TwoEltSlashSlash) {
    http_req_t::resource_t resource("/foo/bar//");
    auto it = resource.begin();
    auto jt = resource.end();
    ASSERT_EQ("foo", *it);
    ++it;
    ASSERT_EQ("bar", *it);
    ++it;
    ASSERT_NE(jt, it);
    ASSERT_EQ("", *it);
    ++it;
    ASSERT_NE(jt, it);
    ASSERT_EQ("", *it);
    ++it;
    ASSERT_EQ(jt, it);
}


TEST(HttpResourceTest, TwoEltSlashSlashSlash) {
    http_req_t::resource_t resource("/foo/bar///");
    auto it = resource.begin();
    auto jt = resource.end();
    ASSERT_EQ("foo", *it);
    ++it;
    ASSERT_EQ("bar", *it);
    ++it;
    ASSERT_NE(jt, it);
    ASSERT_EQ("", *it);
    ++it;
    ASSERT_NE(jt, it);
    ASSERT_EQ("", *it);
    ++it;
    ASSERT_NE(jt, it);
    ASSERT_EQ("", *it);
    ++it;
    ASSERT_EQ(jt, it);
}

TEST(HttpResourceTest, TwoEltSlashSlashElt) {
    http_req_t::resource_t resource("/foo/bar//baz");
    auto it = resource.begin();
    auto jt = resource.end();
    ASSERT_EQ("foo", *it);
    ++it;
    ASSERT_EQ("bar", *it);
    ++it;
    ASSERT_NE(jt, it);
    ASSERT_EQ("", *it);
    ++it;
    ASSERT_EQ("baz", *it);
    ++it;
    ASSERT_EQ(jt, it);
}

TEST(HttpResourceTest, SlashSlash) {
    http_req_t::resource_t resource("//");
    auto it = resource.begin();
    auto jt = resource.end();
    ASSERT_NE(jt, it);
    ASSERT_EQ("", *it);
    ++it;
    ASSERT_EQ(jt, it);
}



}  // namespace unittest
