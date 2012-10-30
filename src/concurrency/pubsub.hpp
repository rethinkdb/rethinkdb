// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_PUBSUB_HPP_
#define CONCURRENCY_PUBSUB_HPP_

#include "concurrency/mutex_assertion.hpp"
#include "containers/intrusive_list.hpp"
#include "utils.hpp"
#include "arch/runtime/runtime_utils.hpp"

/* Forward declaration */

template<class subscriber_t>
class publisher_controller_t;

/* `publisher_t` is used to implement small-scale pub/sub architectures. It's
useful whenever you need to broadcast notifications to some number of listeners.

Each publisher has zero or more subscriptions. Each subscription is associated
with a subscriber. A subscription is usually associated with one publisher, but
it is possible to unassociate a subscription or to move it from one publisher to
another.

The publisher handles concurrency issues for you. In particular, it won't let
you add or remove subscribers or destroy the publisher while in the middle of
delivering a notification. */

template<class subscriber_t>
class publisher_t : public home_thread_mixin_t {
public:
    class subscription_t : public intrusive_list_node_t<subscription_t>, public home_thread_mixin_debug_only_t {
    public:
        /* Construct a `subscription_t` that is not subscribed to any publisher.
        */
        explicit subscription_t(subscriber_t sub) : subscriber(sub), publisher(NULL) { }

        /* Construct a `subscription_t` and subscribe to the given publisher. */
        subscription_t(subscriber_t sub, publisher_t *pub) : subscriber(sub), publisher(NULL) {
            reset(pub);
        }

        /* Cause us to be subscribed to the given publisher (if any) and not to
        any other publisher. */
        void reset(publisher_t *pub = NULL) {
            if (publisher) {
                mutex_assertion_t::acq_t acq(&publisher->mutex);
                publisher->subscriptions.remove(this);
            }
            publisher = pub;
            if (publisher) {
                mutex_assertion_t::acq_t acq(&publisher->mutex);
                publisher->subscriptions.push_back(this);
            }
        }

        /* Unsubscribe from the publisher you are subscribed to, if any. */
        ~subscription_t() {
            reset();
        }

        subscriber_t subscriber;

    private:
        friend class publisher_controller_t<subscriber_t>;

        publisher_t *publisher;

        DISABLE_COPYING(subscription_t);
    };

    void rethread(int new_thread) {
        real_home_thread = new_thread;
        mutex.rethread(new_thread);
    }

private:
    friend class subscription_t;
    friend class publisher_controller_t<subscriber_t>;

    publisher_t() { }
    ~publisher_t() {
        rassert(subscriptions.empty());
    }

    intrusive_list_t<subscription_t> subscriptions;
    mutex_assertion_t mutex;

    DISABLE_COPYING(publisher_t);
};

/* To create a publisher, create a `publisher_controller_t` and call its
`get_publisher()` method. `publisher_t` is split from `publisher_controller_t`
so that you can pass a `publisher_t *` to things; they will be able to listen
for events, but not publish events themselves.

To publish things, call `publish()` on the `publisher_controller_t` you
created. `publish()` takes a function, which will be called once for each
subscription. The function must not block. */

template<class subscriber_t>
class publisher_controller_t : public home_thread_mixin_t {
public:
    publisher_controller_t() { }

    publisher_t<subscriber_t> *get_publisher() {
        return &publisher;
    }

    template<class callable_t>
    void publish(const callable_t &callable) {
        mutex_assertion_t::acq_t acq(&publisher.mutex);
        for (typename publisher_t<subscriber_t>::subscription_t *sub = publisher.subscriptions.head();
             sub;
             sub = publisher.subscriptions.next(sub)) {
            /* `callable()` should not block */
            ASSERT_FINITE_CORO_WAITING;
            callable(sub->subscriber);
        }
    }

    void rethread(int new_thread) {
        rassert(publisher.subscriptions.empty(),
                "Cannot rethread a `publisher_t` that has subscribers.");
        publisher.rethread(new_thread);
        real_home_thread = new_thread;
    }

private:
    publisher_t<subscriber_t> publisher;
    DISABLE_COPYING(publisher_controller_t);
};

#endif /* CONCURRENCY_PUBSUB_HPP_ */
