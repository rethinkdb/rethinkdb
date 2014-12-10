// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "parsing/utf8.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

TEST(UTF8ValidationTest, EmptyStrings) {
    ASSERT_TRUE(utf8::is_valid(""));
    ASSERT_TRUE(utf8::is_valid(std::string("")));
    ASSERT_TRUE(utf8::is_valid(datum_string_t("")));
}

TEST(UTF8ValidationTest, SimplePositives) {
    ASSERT_TRUE(utf8::is_valid("foo"));
    ASSERT_TRUE(utf8::is_valid(std::string("foo")));
    ASSERT_TRUE(utf8::is_valid(datum_string_t("foo")));
}

TEST(UTF8ValidationTest, ValidSurrogates) {
    // U+0024 $
    ASSERT_TRUE(utf8::is_valid("foo$"));
    // U+00A2 cent sign
    ASSERT_TRUE(utf8::is_valid("foo\xc2\xa2"));
    // U+20AC euro sign
    ASSERT_TRUE(utf8::is_valid("foo\xe2\x82\xac"));
    // U+10348 hwair
    ASSERT_TRUE(utf8::is_valid("foo\xf0\x90\x8d\x88"));

    // From RFC 3629 examples:
    // U+0041 U+2262 U+0391 U+002E A<NOT IDENTICAL TO><ALPHA>
    ASSERT_TRUE(utf8::is_valid("\x41\xe2\x89\xa2\xce\x91\x2e"));
    // U+D55C U+AD6D U+C5B4 Korean "hangugeo", the Korean language
    ASSERT_TRUE(utf8::is_valid("\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4"));
    // U+65E5 U+672C U+8A9E Japanese "nihongo", the Japanese language
    ASSERT_TRUE(utf8::is_valid("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e"));
    // U+233B4 Chinese character meaning 'stump of a tree' prefixed with a UTF-8 BOM
    ASSERT_TRUE(utf8::is_valid("\xef\xbb\xbf\xf0\xa3\x8e\xb4"));
}

TEST(UTF8ValidationTest, InvalidCharacters) {
    // totally incoherent
    ASSERT_FALSE(utf8::is_valid("\xff"));
    // also illegal
    ASSERT_FALSE(utf8::is_valid("\xc0"));
    ASSERT_FALSE(utf8::is_valid("\xc1"));
    ASSERT_FALSE(utf8::is_valid("\xf5"));
    // continuation byte with no leading byte
    ASSERT_FALSE(utf8::is_valid("\xbf"));
    // two byte character with two continuation bytes
    ASSERT_FALSE(utf8::is_valid("\xc2\xa2\xbf"));
    // two byte character with no continuation bytes
    ASSERT_FALSE(utf8::is_valid("\xc2"));
    // three byte leader, then two byte surrogate
    ASSERT_FALSE(utf8::is_valid("\xe0\xc2\xa2"));
}

TEST(UTF8ValidationTest, NullBytes) {
    ASSERT_TRUE(utf8::is_valid("foo\x00\xff")); // C string :/
    // these two are a correct string, with a null byte, and then an
    // invalid character; we're verifying that parsing proceeds past
    // the NULL.
    ASSERT_FALSE(utf8::is_valid(std::string("foo\x00\xff", 5)));
    ASSERT_FALSE(utf8::is_valid(datum_string_t(std::string("foo\x00\xff", 5))));
}

TEST(UTF8ValidationTest, IllegalCharacters) {
    // ASCII $ encoded as two characters
    ASSERT_FALSE(utf8::is_valid("foo\xc0\xa4"));
    // U+00A2 cent sign encoded as three characters
    ASSERT_FALSE(utf8::is_valid("foo\xe0\x82\xa2"));
    // U+20AC cent sign encoded as four characters
    ASSERT_FALSE(utf8::is_valid("foo\xf0\e2\x82\xac"));
    // what would be U+2134AC if that existed
    ASSERT_FALSE(utf8::is_valid("foo\xf8\x88\x93\x92\xac"));
    // NULL encoded as two characters ("modified UTF-8")
    ASSERT_FALSE(utf8::is_valid("foo\xc0\x80"));
}

};
