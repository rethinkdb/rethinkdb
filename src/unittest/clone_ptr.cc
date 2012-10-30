// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "containers/clone_ptr.hpp"

namespace unittest {

class thing_t {
public:
    explicit thing_t(int i) : value(i) { }
    virtual ~thing_t() { }
    virtual thing_t *clone() const {
        return new thing_t(value);
    }
    virtual int get_foo() {
        return value + 2;
    }
    int value;
};

class thing2_t : public thing_t {
public:
    thing2_t(int i, int j) : thing_t(i), value2(j) { }
    virtual ~thing2_t() { }
    virtual thing2_t *clone() const {
        return new thing2_t(value, value2);
    }
    virtual int get_foo() {
        return value + value2 + 8;
    }
    int value2;
};

TEST(ClonePtrTest, MakeAndCopy) {
    clone_ptr_t<thing_t> p;
    p = clone_ptr_t<thing_t>(new thing_t(3));
    EXPECT_EQ(3, p->value);
    clone_ptr_t<thing_t> p2 = p;
    p = clone_ptr_t<thing_t>(NULL);
    EXPECT_EQ(3, p2->value);
}

TEST(ClonePtrTest, BooleanConversion) {
    clone_ptr_t<thing_t> p(new thing_t(3));
    if (!p) {
        ADD_FAILURE() << "boolean operator broken";
    }
    if (clone_ptr_t<thing_t>()) {
        ADD_FAILURE() << "boolean operator broken";
    }
}

TEST(ClonePtrTest, Casting) {
    clone_ptr_t<thing2_t> x(new thing2_t(6, 8));
    EXPECT_EQ(22, x->get_foo());
    clone_ptr_t<thing_t> y(x);
    EXPECT_EQ(22, y->get_foo());
}

}  // namespace unittest
