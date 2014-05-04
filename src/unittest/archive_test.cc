// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <string>

#include "unittest/gtest.hpp"

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"

namespace unittest {

void dump_to_string(write_message_t *wm, std::string *out) {
    intrusive_list_t<write_buffer_t> *buffers = wm->unsafe_expose_buffers();

    out->clear();
    for (write_buffer_t *p = buffers->head(); p; p = buffers->next(p)) {
        out->append(p->data, p->data + p->size);
    }
}

TEST(WriteMessageTest, Variant) {
    boost::variant<int32_t, std::string, int8_t> v("Hello, world!");

    write_message_t wm;

    serialize(&wm, v);

    std::string s;
    dump_to_string(&wm, &s);

    ASSERT_EQ(2, s[0]);
    ASSERT_EQ(13, s[1]);  // The varint-encoded string length.
    ASSERT_EQ('H', s[2]);
    ASSERT_EQ('!', s[14]);
    ASSERT_EQ(15u, s.size());
}



}  // namespace unittest
