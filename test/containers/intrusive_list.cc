
#include <retest.hpp>
#include "containers/intrusive_list.hpp"

struct my_t : public intrusive_list_node_t<my_t> {
    my_t(int _value) : value(_value) {}
    int value;
};

void test_back() {
    my_t m0(0), m1(1), m2(2);

    // Insert some elements
    intrusive_list_t<my_t> list;
    list.push_back(&m0);
    list.push_back(&m1);
    list.push_back(&m2);

    // Check the list
    assert_eq(list.head(), &m0);
    assert_eq(list.tail(), &m2);
    assert_eq(m0.prev, (my_t*)NULL);
    assert_eq(m0.next, &m1);
    assert_eq(m1.prev, &m0);
    assert_eq(m1.next, &m2);
    assert_eq(m2.prev, &m1);
    assert_eq(m2.next, (my_t*)NULL);
}

void test_front() {
    my_t m0(0), m1(1), m2(2);

    // Insert some elements
    intrusive_list_t<my_t> list;
    list.push_front(&m2);
    list.push_front(&m1);
    list.push_front(&m0);

    // Check the list
    assert_eq(list.head(), &m0);
    assert_eq(list.tail(), &m2);
    assert_eq(m0.prev, (my_t*)NULL);
    assert_eq(m0.next, &m1);
    assert_eq(m1.prev, &m0);
    assert_eq(m1.next, &m2);
    assert_eq(m2.prev, &m1);
    assert_eq(m2.next, (my_t*)NULL);
}

void test_remove_head() {
    my_t m0(0), m1(1), m2(2);

    // Insert some elements
    intrusive_list_t<my_t> list;
    list.push_back(&m0);
    list.push_back(&m1);
    list.push_back(&m2);

    // Remove head
    list.remove(&m0);

    // Check the list
    assert_eq(list.head(), &m1);
    assert_eq(list.tail(), &m2);
    assert_eq(m1.prev, (my_t*)NULL);
    assert_eq(m1.next, &m2);
    assert_eq(m2.prev, &m1);
    assert_eq(m2.next, (my_t*)NULL);
}

void test_remove_tail() {
    my_t m0(0), m1(1), m2(2);

    // Insert some elements
    intrusive_list_t<my_t> list;
    list.push_back(&m0);
    list.push_back(&m1);
    list.push_back(&m2);

    // Remove head
    list.remove(&m2);

    // Check the list
    assert_eq(list.head(), &m0);
    assert_eq(list.tail(), &m1);
    assert_eq(m0.prev, (my_t*)NULL);
    assert_eq(m0.next, &m1);
    assert_eq(m1.prev, &m0);
    assert_eq(m1.next, (my_t*)NULL);
}
