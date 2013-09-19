// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"
#include "rdb_protocol/datum.hpp"

namespace unittest {
void test_mangle(const std::string &pkey, const std::string &skey, boost::optional<uint64_t> tag = boost::optional<size_t>()) {
    std::string tag_string;
    if (tag) {
        tag_string = std::string(reinterpret_cast<const char *>(&*tag), sizeof(uint64_t));
    }
    std::string mangled = ql::datum_t::mangle_secondary(skey, pkey, tag_string);
    ASSERT_EQ(pkey, ql::datum_t::extract_primary(mangled));
    ASSERT_EQ(skey, ql::datum_t::extract_secondary(mangled));
    ASSERT_EQ(tag, ql::datum_t::extract_tag(mangled));
}

TEST(PrintSecondary, Mangle) {
    test_mangle("foo", "bar", 1);
    test_mangle("foo", "bar");
    test_mangle("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
                "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
                100000);
    test_mangle("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
                "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
}
};
