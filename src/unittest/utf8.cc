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
    ASSERT_EQ(0, reason.position);
    ASSERT_STREQ("Expected continuation byte, saw end of string", reason.explanation);

    // three byte leader, then two byte surrogate
    ASSERT_FALSE(utf8::is_valid("\xe0\xc2\xa2"));

    ASSERT_FALSE(utf8::is_valid("\xe0\xc2\xa2", &reason));
    ASSERT_EQ(0, reason.position);
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

// Stress test per http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html
TEST(UTF8ValidationStressTest, CorrectString) {
    ASSERT_TRUE(utf8::is_valid(u8"κόσμε"));
}

TEST(UTF8ValidationStressTest, FirstPossibleSequence) {
    ASSERT_TRUE(utf8::is_valid(u8"\U00000000"));
    ASSERT_TRUE(utf8::is_valid(u8"\U00000080"));
    ASSERT_TRUE(utf8::is_valid(u8"\U00000800"));
    ASSERT_TRUE(utf8::is_valid(u8"\U00010000"));
}

TEST(UTF8ValidationStressTest, LastPossibleSequence) {
    ASSERT_TRUE(utf8::is_valid(u8"\U0000007F"));
    ASSERT_TRUE(utf8::is_valid(u8"\U000007FF"));
    ASSERT_TRUE(utf8::is_valid(u8"\U0000FFFF"));
    ASSERT_TRUE(utf8::is_valid(u8"\U0010FFFF"));
}

TEST(UTF8ValidationStressTest, MiscBoundaryConditions) {
    ASSERT_TRUE(utf8::is_valid("\xed\x9f\xbf")); // U+0000D7FF
    ASSERT_TRUE(utf8::is_valid("\xee\x80\x80")); // U+0000E000
    ASSERT_TRUE(utf8::is_valid("\xef\xbf\xbd")); // U+0000FFFD
    ASSERT_TRUE(utf8::is_valid("\xf4\x8f\xbf\xbf"));  // U+0010FFFF
    ASSERT_FALSE(utf8::is_valid("\xf4\x90\x80\x80")); // U+00110000
}

TEST(UTF8ValidationStressTest, ImpossibleBytes) {
    ASSERT_FALSE(utf8::is_valid("\xfe"));
    ASSERT_FALSE(utf8::is_valid("\xff"));
    ASSERT_FALSE(utf8::is_valid("\xfe\xfe\xff\xff"));
}

TEST(UTF8ValidationStressTest, OverlongSequences) {
    ASSERT_FALSE(utf8::is_valid("\xc0\xaf"));
    ASSERT_FALSE(utf8::is_valid("\xe0\x80\xaf"));
    ASSERT_FALSE(utf8::is_valid("\xf0\x80\x80\xaf"));
    ASSERT_FALSE(utf8::is_valid("\xf8\x80\x80\x80\xaf"));

    ASSERT_FALSE(utf8::is_valid("\xc1\xbf"));
    ASSERT_FALSE(utf8::is_valid("\xe0\x9f\xbf"));
    ASSERT_FALSE(utf8::is_valid("\xf0\x8f\xbf\xbf"));
    ASSERT_FALSE(utf8::is_valid("\xf8\x87\xbf\xbf\xbf"));

    ASSERT_FALSE(utf8::is_valid("\xc0\x80"));
    ASSERT_FALSE(utf8::is_valid("\xe0\x80\x80"));
    ASSERT_FALSE(utf8::is_valid("\xf0\x80\x80\x80"));
    ASSERT_FALSE(utf8::is_valid("\xf8\x80\x80\x80\x80"));
}

void AllInvalidString(const char *start, const char *end) {
    utf8::reason_t reason;
    char32_t codepoint;
    const char *cbegin = start;
    const char *cend = start;
    size_t count = 0;
    while (cend != end) {
        cend = utf8::next_codepoint(cbegin, end, &codepoint, &reason);
        ASSERT_EQ(U'\uFFFD', codepoint);
        count++;
        cbegin = cend;
    }
    ASSERT_EQ(end - start, count);
}

void AllInvalidString(const char *start) {
    AllInvalidString(start, start + strlen(start));
}

TEST(UTF8ValidationStressTest, UnexpectedContinuationBytes) {
    {
        SCOPED_TRACE("First continuation byte");
        AllInvalidString("\x80");
    }
    {
        SCOPED_TRACE("Last continuation byte");
        AllInvalidString("\xbf");
    }
    {
        SCOPED_TRACE("2 continuation bytes");
        AllInvalidString("\x80\x80");
    }
    {
        SCOPED_TRACE("3 continuation bytes");
        AllInvalidString("\x80\x80\x80");
    }
    {
        SCOPED_TRACE("4 continuation bytes");
        AllInvalidString("\x80\x80\x80\x80");
    }
    {
        SCOPED_TRACE("5 continuation bytes");
        AllInvalidString("\x80\x80\x80\x80\x80");
    }
    {
        SCOPED_TRACE("6 continuation bytes");
        AllInvalidString("\x80\x80\x80\x80\x80\x80");
    }
    {
        SCOPED_TRACE("7 continuation bytes");
        AllInvalidString("\x80\x80\x80\x80\x80\x80\x80");
    }
    {
        SCOPED_TRACE("All possible continuation bytes");
        char string[65];
        for (int i = 0; i < 65; i++) {
            string[i] = '\x80' + i;
        }
        string[64] = '\0';
        AllInvalidString(string);
    }
}

void LonelyString(const char *start, const char *end) {
    utf8::reason_t reason;
    char32_t codepoint = 0;
    const char *cbegin = start;
    const char *cend = start;
    size_t count = 0;
    while (cend != end) {
        cend = utf8::next_codepoint(cbegin, end, &codepoint, &reason);
        if ((count % 2) == 0) {
            ASSERT_EQ(U'\uFFFD', codepoint);
        } else {
            ASSERT_EQ(U' ', codepoint);
        }
        count++;
        cbegin = cend;
    }
    ASSERT_EQ(strlen(start), count);
}

void LonelyString(const char *start) {
    LonelyString(start, start + strlen(start));
}

TEST(UTF8ValidationStressTest, LonelyStartCharacters) {
    {
        SCOPED_TRACE("Two-byte sequences");
        char string[65];
        for (int i = 0; i < 64; i++) {
            if ((i % 2) == 0) {
                string[i] = '\xc0' + (i / 2);
            } else {
                string[i] = ' ';
            }
        }
        string[64] = '\0';
        LonelyString(string);
    }
    {
        SCOPED_TRACE("Three-byte sequences");
        char string[33];
        for (int i = 0; i < 32; i++) {
            if ((i % 2) == 0) {
                string[i] = '\xe0' + (i / 2);
            } else {
                string[i] = ' ';
            }
        }
        string[32] = '\0';
        LonelyString(string);
    }
    {
        SCOPED_TRACE("Four-byte sequences");
        char string[17];
        for (int i = 0; i < 16; i++) {
            if ((i % 2) == 0) {
                string[i] = '\xf0' + (i / 2);
            } else {
                string[i] = ' ';
            }
        }
        string[16] = '\0';
        LonelyString(string);
    }
}

void NBadCodepoints(const char *start, size_t n) {
    const char *end = start + strlen(start);
    const char *cbegin = start;
    const char *cend = start;
    utf8::reason_t reason;
    char32_t codepoint;
    ASSERT_NE(start, end);
    size_t count = 0;
    while (cend != end) {
        cend = utf8::next_codepoint(cbegin, end, &codepoint, &reason);
        ASSERT_EQ(U'\uFFFD', codepoint);
        count++;
        cbegin = cend;
    }
    ASSERT_EQ(n, count);
}

void SingleBadCodepoint(const char *start) {
    NBadCodepoints(start, 1);
}

TEST(UTF8ValidationStressTest, MissingLastContinuationByte) {
    {
        SCOPED_TRACE("Two-byte sequences (U+0000)");
        SingleBadCodepoint("\xc0");
    }
    {
        SCOPED_TRACE("Three-byte sequences (U+0000)");
        SingleBadCodepoint("\xe0\x80");
    }
    {
        SCOPED_TRACE("Four-byte sequences (U+0000)");
        SingleBadCodepoint("\xf0\x80\x80");
    }
    {
        SCOPED_TRACE("Two-byte sequences (U+07FF)");
        SingleBadCodepoint("\xdf");
    }
    {
        SCOPED_TRACE("Three-byte sequences (U+FFFF)");
        SingleBadCodepoint("\xef\xbf");
    }
    {
        SCOPED_TRACE("Four-byte sequences (U-0010FFFF)");
        SingleBadCodepoint("\xf4\x8f\xbf");
    }
    {
        SCOPED_TRACE("Six concatenated sequences");
        NBadCodepoints("\xc0\xe0\x80\xf0\x80\x80\xdf\xef\xbf\xf4\x8f\xbf", 6);
    }
}

TEST(UTF8CodepointIterationTest, SimpleString) {
    std::string demo = "this is a demonstration string";
    utf8::reason_t reason;
    char32_t codepoint;

    auto start = demo.cbegin();
    auto end = demo.cend();
    auto cend = utf8::next_codepoint(start, end, &codepoint, &reason);
    ASSERT_NE(cend, end);
    ASSERT_STREQ("", reason.explanation);
    ASSERT_EQ(U't', codepoint);
    ASSERT_EQ("t", std::string(start, cend));
    ASSERT_EQ(start + 1, cend);
    start = cend;
    cend = utf8::next_codepoint(start, end, &codepoint, &reason);
    ASSERT_NE(cend, end);
    ASSERT_STREQ("", reason.explanation);
    ASSERT_EQ(U'h', codepoint);
    ASSERT_EQ("h", std::string(start, cend));
    ASSERT_EQ(start + 1, cend);
    for (int i = 0; i < 10; i++) {
        start = cend;
        cend = utf8::next_codepoint(start, end, &codepoint, &reason);
        ASSERT_EQ(start + 1, cend);
    }
    ASSERT_NE(cend, end);
    ASSERT_STREQ("", reason.explanation);
    ASSERT_EQ(U'e', codepoint);
    ASSERT_EQ("e", std::string(start, cend));
    for (int i = 0; i < 10; i++) {
        start = cend;
        cend = utf8::next_codepoint(start, end, &codepoint, &reason);
        ASSERT_EQ(start + 1, cend);
    }
    ASSERT_NE(cend, end);
    ASSERT_STREQ("", reason.explanation);
    ASSERT_EQ(U'o', codepoint);
    ASSERT_EQ("o", std::string(start, cend));
    for (int i = 0; i < 10; i++) {
        start = cend;
        cend = utf8::next_codepoint(start, end, &codepoint, &reason);
    }
    ASSERT_EQ(cend, end);
    ASSERT_STREQ("", reason.explanation);
}

TEST(UTF8CodepointIterationTest, Zalgo) {
    const char zalgo[] = "H\xcd\x95"
        "a\xcc\x95\xcd\x8d\xcc\x99\xcd\x8d\xcc\xab\xcd\x87\xcc\xa5\xcc\xa3"
        "v\xcc\xb4"
        "e\xcd\x98\xcc\x96\xcc\xb1\xcd\x96"
        " \xcd\xa1\xcc\xac"
        "s\xcd\x8e\xcc\xa5\xcc\xba\xcd\x88\xcc\xab"
        "o\xcc\xa3\xcc\xb3\xcc\xae\xcd\x85\xcc\xa9"
        "m\xcd\xa2\xcd\x94\xcc\x9e\xcc\x99\xcd\x99\xcc\x9c"
        "e"
        " \xcc\xa5"
        "Z\xcc\xb6"
        "a\xcc\xab\xcc\xa9\xcd\x8e\xcc\xb2\xcc\xac\xcc\xba"
        "l\xcc\x98\xcd\x87\xcd\x94"
        "g\xcc\xb6\xcc\x9e\xcd\x99\xcc\xbc"
        "o"
        ".\xcc\x9b\xcc\xab\xcc\xa9";
    const char32_t zalgo_codepoints[] = U"\u0048\u0355\u0061\u0315\u034d\u0319\u034d"
        U"\u032b\u0347\u0325\u0323\u0076\u0334\u0065\u0358\u0316\u0331\u0356\u0020\u0361"
        U"\u032c\u0073\u034e\u0325\u033a\u0348\u032b\u006f\u0323\u0333\u032e\u0345\u0329"
        U"\u006d\u0362\u0354\u031e\u0319\u0359\u031c\u0065\u0020\u0325\u005a\u0336\u0061"
        U"\u032b\u0329\u034e\u0332\u032c\u033a\u006c\u0318\u0347\u0354\u0067\u0336\u031e"
        U"\u0359\u033c\u006f\u002e\u031b\u032b\u0329";
    auto start = zalgo;
    auto end = zalgo + sizeof(zalgo) - 1;
    const char32_t *current = zalgo_codepoints;
    char32_t codepoint;
    utf8::reason_t reason;
    size_t seen = 0;
    while (start != end) {
        auto cend = utf8::next_codepoint(start, end, &codepoint, &reason);
        ASSERT_EQ(*current, codepoint);
        start = cend;
        ++current;
        ++seen;
    }
    ASSERT_EQ(66, seen);
}
TEST(UTF8IterationTest, SimpleString) {
    std::string demo = "this is a demonstration string";
    utf8::string_iterator_t it(demo);
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U't', *it);
    it++;
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U'h', *it);
    std::advance(it, 10);
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U'e', *it);
    std::advance(it, 10);
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U'o', *it);
    std::advance(it, 10);
    ASSERT_TRUE(it.is_done());
}

TEST(UTF8IterationTest, SimpleStringNormalIteration) {
    std::string demo = "this is a demonstration string";
    utf8::string_iterator_t it(demo);
    utf8::string_iterator_t end(utf8::string_iterator_t::make_end(demo));
    auto following = demo.begin();
    while (it != end) {
        ASSERT_EQ(*following++, *it++);
    }
    ASSERT_TRUE(it.is_done());
}

TEST(UTF8IterationTest, SimpleCString) {
    const char *str = "this is a demonstration string";
    utf8::array_iterator_t it(str, str + strlen(str));
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U't', *it);
    it++;
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U'h', *it);
    std::advance(it, 10);
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U'e', *it);
    std::advance(it, 10);
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U'o', *it);
    std::advance(it, 10);
    ASSERT_TRUE(it.is_done());
}

TEST(UTF8IterationTest, SimpleStringRange) {
    std::string demo = "this is a demonstration string";

    utf8::string_iterator_t it(demo.begin(), demo.end());
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U't', *it);
    it++;
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U'h', *it);
    std::advance(it, 10);
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U'e', *it);
    std::advance(it, 10);
    ASSERT_FALSE(it.is_done());
    ASSERT_EQ(U'o', *it);
    std::advance(it, 10);
    ASSERT_TRUE(it.is_done());
}

TEST(UTF8IterationTest, EmptyString) {
    {
        std::string s;
        utf8::string_iterator_t it(utf8::string_iterator_t::make_end(s));
        ASSERT_TRUE(it.is_done());
    }

    {
        const char *c = "";
        utf8::array_iterator_t it(c, c);
        ASSERT_TRUE(it.is_done());
    }

    {
        std::string s = "";
        utf8::string_iterator_t it(s.begin(), s.end());
        ASSERT_TRUE(it.is_done());
    }
}

// if we can handle this, we can probably handle anything
TEST(UTF8IterationTest, Zalgo) {
    const char zalgo[] = "H\xcd\x95"
        "a\xcc\x95\xcd\x8d\xcc\x99\xcd\x8d\xcc\xab\xcd\x87\xcc\xa5\xcc\xa3"
        "v\xcc\xb4"
        "e\xcd\x98\xcc\x96\xcc\xb1\xcd\x96"
        " \xcd\xa1\xcc\xac"
        "s\xcd\x8e\xcc\xa5\xcc\xba\xcd\x88\xcc\xab"
        "o\xcc\xa3\xcc\xb3\xcc\xae\xcd\x85\xcc\xa9"
        "m\xcd\xa2\xcd\x94\xcc\x9e\xcc\x99\xcd\x99\xcc\x9c"
        "e"
        " \xcc\xa5"
        "Z\xcc\xb6"
        "a\xcc\xab\xcc\xa9\xcd\x8e\xcc\xb2\xcc\xac\xcc\xba"
        "l\xcc\x98\xcd\x87\xcd\x94"
        "g\xcc\xb6\xcc\x9e\xcd\x99\xcc\xbc"
        "o"
        ".\xcc\x9b\xcc\xab\xcc\xa9";
    const char32_t zalgo_codepoints[] = U"\u0048\u0355\u0061\u0315\u034d\u0319\u034d"
        U"\u032b\u0347\u0325\u0323\u0076\u0334\u0065\u0358\u0316\u0331\u0356\u0020\u0361"
        U"\u032c\u0073\u034e\u0325\u033a\u0348\u032b\u006f\u0323\u0333\u032e\u0345\u0329"
        U"\u006d\u0362\u0354\u031e\u0319\u0359\u031c\u0065\u0020\u0325\u005a\u0336\u0061"
        U"\u032b\u0329\u034e\u0332\u032c\u033a\u006c\u0318\u0347\u0354\u0067\u0336\u031e"
        U"\u0359\u033c\u006f\u002e\u031b\u032b\u0329";
    utf8::array_iterator_t it(zalgo, zalgo + strlen(zalgo));
    utf8::array_iterator_t end(utf8::array_iterator_t::make_end(zalgo + strlen(zalgo)));
    const char32_t *current = zalgo_codepoints;
    size_t seen = 0;
    while (it != end) {
        ASSERT_EQ(*current, *it);
        ++current;
        ++it;
        ++seen;
    }
    ASSERT_EQ(66, seen);
}

} // namespace unittest
