#include "unittest/gtest.hpp"

#include "containers/uuid.hpp"

namespace unittest {

TEST(UuidTest, UuidToStr) {
    std::string s = "f47ac10b-58cc-4372-a567-0e02b2c3d479";
    uuid_t x = str_to_uuid(s);
    std::string t = uuid_to_str(x);

    ASSERT_EQ(s, t);
}

}  // namespace unittest
