#include <string>

#include "unittest/gtest.hpp"

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"

namespace unittest {

void dump_to_string(write_message_t *msg, std::string *out) {
    intrusive_list_t<write_buffer_t> *buffers = msg->unsafe_expose_buffers();

    out->clear();
    for (write_buffer_t *p = buffers->head(); p; p = buffers->next(p)) {
        out->append(p->data, p->data + p->size);
    }
}

TEST(WriteMessageTest, Variant) {
    boost::variant<int32_t, std::string, int8_t> v("Hello, world!");

    write_message_t msg;

    msg << v;

    std::string s;
    dump_to_string(&msg, &s);

    std::vector<char> u(s.begin(), s.end());

    ASSERT_EQ(2, u[0]);
    ASSERT_EQ(13, *reinterpret_cast<uint64_t *>(u.data() + 1));
    ASSERT_EQ('H', u[9]);
    ASSERT_EQ('!', u[21]);
    ASSERT_EQ(22, u.size());
}



}  // namespace unittest
