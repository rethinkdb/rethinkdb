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
    utf8::reason_t reason;

    // totally incoherent
    ASSERT_FALSE(utf8::is_valid("\xff"));

    ASSERT_FALSE(utf8::is_valid("\xff", &reason));
    ASSERT_EQ(0, reason.position);
    ASSERT_STREQ("Invalid initial byte seen", reason.explanation);
    // also illegal
    ASSERT_FALSE(utf8::is_valid("\xc0\xa2""foo"));
    ASSERT_FALSE(utf8::is_valid("\xc1\xa2""foo"));
    ASSERT_FALSE(utf8::is_valid("\xf5\xa2\xa2\xa2""bar"));

    ASSERT_FALSE(utf8::is_valid("\xc0\xa2""foo", &reason));
    ASSERT_EQ(1, reason.position);
    ASSERT_STREQ("Overlong encoding seen", reason.explanation);
    ASSERT_FALSE(utf8::is_valid("\xc1\xa2""foo", &reason));
    ASSERT_EQ(1, reason.position);
    ASSERT_STREQ("Overlong encoding seen", reason.explanation);
    ASSERT_FALSE(utf8::is_valid("\xf5\xa2\xa2\xa2""bar", &reason));
    ASSERT_EQ(3, reason.position);
    ASSERT_STREQ("Non-Unicode character encoded (beyond U+10FFFF)", reason.explanation);

    // continuation byte with no leading byte
    ASSERT_FALSE(utf8::is_valid("\xbf"));

    ASSERT_FALSE(utf8::is_valid("\xbf", &reason));
    ASSERT_EQ(0, reason.position);
    ASSERT_STREQ("Invalid initial byte seen", reason.explanation);

    // two byte character with two continuation bytes
    ASSERT_FALSE(utf8::is_valid("\xc2\xa2\xbf"));

    ASSERT_FALSE(utf8::is_valid("\xc2\xa2\xbf", &reason));
    ASSERT_EQ(2, reason.position);
    ASSERT_STREQ("Invalid initial byte seen", reason.explanation);

    // two byte character with no continuation bytes
    ASSERT_FALSE(utf8::is_valid("\xc2"));

    ASSERT_FALSE(utf8::is_valid("\xc2", &reason));
    ASSERT_EQ(1, reason.position);
    ASSERT_STREQ("Expected continuation byte, saw end of string", reason.explanation);

    // three byte leader, then two byte surrogate
    ASSERT_FALSE(utf8::is_valid("\xe0\xc2\xa2"));

    ASSERT_FALSE(utf8::is_valid("\xe0\xc2\xa2", &reason));
    ASSERT_EQ(1, reason.position);
    ASSERT_STREQ("Expected continuation byte, saw something else", reason.explanation);
}

TEST(UTF8ValidationTest, NullBytes) {
    utf8::reason_t reason;

    ASSERT_TRUE(utf8::is_valid("foo\x00\xff")); // C string :/
    // these two are a correct string, with a null byte, and then an
    // invalid character; we're verifying that parsing proceeds past
    // the NULL.
    ASSERT_FALSE(utf8::is_valid(std::string("foo\x00\xff", 5)));
    ASSERT_FALSE(utf8::is_valid(datum_string_t(std::string("foo\x00\xff", 5))));

    ASSERT_FALSE(utf8::is_valid(std::string("foo\x00\xff", 5), &reason));
    ASSERT_EQ(4, reason.position);
    ASSERT_STREQ("Invalid initial byte seen", reason.explanation);
    ASSERT_FALSE(utf8::is_valid(datum_string_t(std::string("foo\x00\xff", 5)), &reason));
    ASSERT_EQ(4, reason.position);
    ASSERT_STREQ("Invalid initial byte seen", reason.explanation);
}

TEST(UTF8ValidationTest, IllegalCharacters) {
    utf8::reason_t reason;

    // ASCII $ encoded as two characters
    ASSERT_FALSE(utf8::is_valid("foo\xc0\xa4"));
    // U+00A2 cent sign encoded as three characters
    ASSERT_FALSE(utf8::is_valid("foo\xe0\x82\xa2"));
    // U+20AC cent sign encoded as four characters
    ASSERT_FALSE(utf8::is_valid("foo\xf0\x82\x82\xac"));
    // what would be U+2134AC if that existed
    ASSERT_FALSE(utf8::is_valid("foo\xf8\x88\x93\x92\xac"));
    // NULL encoded as two characters ("modified UTF-8")
    ASSERT_FALSE(utf8::is_valid("foo\xc0\x80"));

    ASSERT_FALSE(utf8::is_valid("foo\xc0\xa4", &reason));
    ASSERT_EQ(4, reason.position);
    ASSERT_STREQ("Overlong encoding seen", reason.explanation);
    ASSERT_FALSE(utf8::is_valid("foo\xe0\x82\xa2", &reason));
    ASSERT_EQ(5, reason.position);
    ASSERT_STREQ("Overlong encoding seen", reason.explanation);
    ASSERT_FALSE(utf8::is_valid("foo\xf0\x82\x82\xac", &reason));
    ASSERT_STREQ("Overlong encoding seen", reason.explanation);
    ASSERT_EQ(6, reason.position);
    ASSERT_FALSE(utf8::is_valid("foo\xf8\x88\x93\x92\xac", &reason));
    ASSERT_EQ(3, reason.position);
    ASSERT_STREQ("Invalid initial byte seen", reason.explanation);
    ASSERT_FALSE(utf8::is_valid("foo\xc0\x80", &reason));
    ASSERT_EQ(4, reason.position);
    ASSERT_STREQ("Overlong encoding seen", reason.explanation);
}

};
