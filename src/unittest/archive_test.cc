#include <string>

#include "unittest/gtest.hpp"

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"

namespace unittest {

TEST(WriteMessageTest, Variant) {
    boost::variant<int, std::string, char> v("Hello, world!");

    write_message_t msg;

    msg << v;


}



}  // namespace unittest
