
#include <retest.hpp>
#include "message_hub.hpp"

struct test_message_t : public cpu_message_t {
    test_message_t(int _key) : key(_key) {}
    int key;
};

void test_message_hub() {
    cpu_message_t
        *m0 = new test_message_t(0),
        *m1 = new test_message_t(1),
        *m2 = new test_message_t(2),
        *m3 = new test_message_t(3),
        *m4 = new test_message_t(4),
        *m5 = new test_message_t(5);
    message_hub_t hub;
    hub.init(0, 2, NULL);
    message_hub_t::msg_list_t l0, l1;
    hub.store_message(0, m0);
    hub.store_message(0, m1);
    hub.store_message(1, m2);
    hub.store_message(1, m3);

    // Make sure queues for CPU 0 and 1 are still empty
    hub.pull_messages(0, &l0);
    assert_eq(l0.empty(), true);
    hub.pull_messages(1, &l1);
    assert_eq(l1.empty(), true);

    // Push the messages out
    hub.push_messages();

    // Make sure queues for CPU 0 and 1 are filled
    hub.pull_messages(0, &l0);
    assert_eq(l0.head(), m0);
    assert_eq(l0.head()->next, m1);
    assert_eq(l0.head()->next->next, (void*)NULL);
    hub.pull_messages(1, &l1);
    assert_eq(l1.head(), m2);
    assert_eq(l1.head()->next, m3);
    assert_eq(l1.head()->next->next, (void*)NULL);

    // Store and push some more messages
    hub.store_message(0, m4);
    hub.store_message(1, m5);
    
    // Make sure queues for CPU 0 and 1 are empty again
    hub.pull_messages(0, &l0);
    assert_eq(l0.empty(), true);
    hub.pull_messages(1, &l1);
    assert_eq(l1.empty(), true);

    // Push new messages
    hub.push_messages();

    // Make sure we only get the new messages
    hub.pull_messages(0, &l0);
    assert_eq(l0.head(), m4);
    assert_eq(l0.head()->next, (void*)NULL);
    hub.pull_messages(1, &l1);
    assert_eq(l1.head(), m5);
    assert_eq(l1.head()->next, (void*)NULL);

    // Free the associated memory
    delete m0;
    delete m1;
    delete m2;
    delete m3;
    delete m4;
    delete m5;
}

