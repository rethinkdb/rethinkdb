#include <arpa/inet.h>

#include "containers/uuid.hpp"
#include "unittest/gtest.hpp"

// We keep the sha1 function hidden to avoid encouraging others from using it.
namespace sha1 {
// hash is a pointer to 20-byte array, unfortunately.
void calc(const void* src, const int bytelength, unsigned char* hash);
}

namespace unittest {

TEST(UuidTest, UuidToStr) {
    std::string s = "f47ac10b-58cc-4372-a567-0e02b2c3d479";
    uuid_t x = str_to_uuid(s);
    std::string t = uuid_to_str(x);

    ASSERT_EQ(s, t);
}


TEST(UuidTest, StrToUuid) {
    // Both upper case and lowercase should be converted the same.
    std::string s_lower = "f47ac10b-58cc-4372-a567-0e02b2c3d479";
    std::string s_upper = "f47ac10b-58CC-4372-a567-0E02B2C3D479";
    uuid_t x = nil_uuid();
    bool success = str_to_uuid(s_lower, &x);
    ASSERT_TRUE(success);
    std::string t = uuid_to_str(x);

    ASSERT_EQ(s_lower, t);

    x = nil_uuid();
    success = str_to_uuid(s_upper, &x);
    ASSERT_TRUE(success);
    t = uuid_to_str(x);

    ASSERT_EQ(s_lower, t);

    // This one is too long.
    std::string u = "f47ac10b-58cc-4372-a567-0e02b2c3d479c";
    uuid_t y;
    bool failure = str_to_uuid(u, &y);
    ASSERT_FALSE(failure);

    // This one has a wrong character in place of '-'.
    std::string v = "f47ac10b-58cc-4372-a567+ge02b2c3d479";
    failure = str_to_uuid(v, &y);
    ASSERT_FALSE(failure);

    // This one has a 'g'.
    std::string w = "f47ac10g-58cc-4372-a567-ge02b2c3d479";
    failure = str_to_uuid(w, &y);
    ASSERT_FALSE(failure);

    // This one is too short.
    std::string w2 = "f47ac10b-58cc-4372-a567-0e02b2c3d47";
    failure = str_to_uuid(w2, &y);
    ASSERT_FALSE(failure);
}

void check_sha(const std::string &str, uint32_t expected[5]) {
    union {
        uint8_t hash[24];
        uint32_t ghetto[6];
    } u;
    u.ghetto[5] = 0xcafeb4b3;

    sha1::calc(str.data(), str.size(), u.hash);

    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(expected[i], htonl(u.ghetto[i]));
    }
    EXPECT_EQ(0xcafeb4b3, u.ghetto[5]);
}

TEST(UuidTest, Sha1) {
    std::string sentence = "The quick brown fox jumps over the lazy dog";
    uint32_t expected[5] = { 0x2fd4e1c6, 0x7a2d28fc, 0xed849ee1, 0xbb76e739, 0x1b93eb12 };

    check_sha(sentence, expected);

    std::string empty = "";
    uint32_t empty_expected[5] = { 0xda39a3ee, 0x5e6b4b0d, 0x3255bfef, 0x95601890, 0xafd80709 };

    check_sha(empty, empty_expected);
}


}  // namespace unittest
