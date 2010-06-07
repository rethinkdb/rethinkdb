
#include <retest.hpp>
#include "message_hub.hpp"

void test_message_hub() {
    message_hub_t hub(2);
    message_hub_t::msg_list_t l0, l1;
    hub.store_message(0, (void*)0);
    hub.store_message(0, (void*)1);
    hub.store_message(1, (void*)2);
    hub.store_message(1, (void*)3);

    // Make sure queues for CPU 0 and 1 are still empty
    hub.pull_messages(0, &l0);
    assert_eq(l0.empty(), true);
    hub.pull_messages(1, &l1);
    assert_eq(l1.empty(), true);

    // Push the messages out
    hub.push_messages();

    // Make sure queues for CPU 0 and 1 are filled
    hub.pull_messages(0, &l0);
    assert_eq(l0.head()->data, (void*)0);
    assert_eq(l0.head()->next->data, (void*)1);
    assert_eq(l0.head()->next->next, (void*)NULL);
    hub.pull_messages(1, &l1);
    assert_eq(l1.head()->data, (void*)2);
    assert_eq(l1.head()->next->data, (void*)3);
    assert_eq(l1.head()->next->next, (void*)NULL);

    // Store and push some more messages
    hub.store_message(0, (void*)4);
    hub.store_message(1, (void*)5);
    
    // Make sure queues for CPU 0 and 1 are empty again
    hub.pull_messages(0, &l0);
    assert_eq(l0.empty(), true);
    hub.pull_messages(1, &l1);
    assert_eq(l1.empty(), true);

    // Push new messages
    hub.push_messages();

    // Make sure we only get the new messages
    hub.pull_messages(0, &l0);
    assert_eq(l0.head()->data, (void*)4);
    assert_eq(l0.head()->next, (void*)NULL);
    hub.pull_messages(1, &l1);
    assert_eq(l1.head()->data, (void*)5);
    assert_eq(l1.head()->next, (void*)NULL);
}

