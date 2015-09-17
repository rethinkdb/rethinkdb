// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"
#include "rdb_protocol/datum.hpp"

namespace unittest {
void test_mangle(const std::string &pkey, const std::string &skey, boost::optional<uint64_t> tag = boost::optional<uint64_t>()) {
    std::string tag_string;
    if (tag) {
        tag_string = std::string(reinterpret_cast<const char *>(&*tag),
                                 sizeof(uint64_t));
    }
    auto versions = {
        reql_version_t::v1_14,
        reql_version_t::v1_16,
        reql_version_t::v2_0,
        reql_version_t::v2_1,
        reql_version_t::v2_2_is_latest
    };
    for (reql_version_t rv : versions) {
        ql::skey_version_t skey_version = ql::skey_version_from_reql_version(rv);
        std::string mangled = ql::datum_t::mangle_secondary(
            skey_version, skey, pkey, tag_string);
        ASSERT_EQ(pkey, ql::datum_t::extract_primary(mangled));
        std::string skey2 = skey;
        guarantee(!(skey2[0] & 0x80)); // None of our types have the top bit set.
        switch (skey_version) {
        case ql::skey_version_t::pre_1_16: break;
        case ql::skey_version_t::post_1_16:
            skey2[0] |= 0x80; // Flip the top bit to indicate 1.16+ skey_version.
            break;
        default: unreachable();
        }
        ASSERT_EQ(skey2, ql::datum_t::extract_secondary(mangled));
        boost::optional<uint64_t> extracted_tag = ql::datum_t::extract_tag(mangled);
        ASSERT_EQ(static_cast<bool>(tag), static_cast<bool>(extracted_tag));
        if (tag) {
            ASSERT_EQ(*tag, *extracted_tag);
        }
    }
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

}  // namespace unittest
