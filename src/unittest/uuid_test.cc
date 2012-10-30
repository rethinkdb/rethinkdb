#include "unittest/gtest.hpp"

#include "containers/uuid.hpp"

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


}  // namespace unittest
