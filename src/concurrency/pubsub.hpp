// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_PUBSUB_HPP_
#define CONCURRENCY_PUBSUB_HPP_

#include "arch/runtime/runtime_utils.hpp"
#include "concurrency/mutex_assertion.hpp"
#include "containers/intrusive_list.hpp"
#include "threading.hpp"

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
class publisher_t {
public:
    class subscription_t : public intrusive_list_node_t<subscription_t> {
    public:
        /* Construct a `subscription_t` that is not subscribed to any publisher.
        */
        explicit subscription_t(subscriber_t sub) : subscriber(sub), publisher(nullptr) { }

        /* Construct a `subscription_t` and subscribe to the given publisher. */
        subscription_t(subscriber_t sub, publisher_t *pub) : subscriber(sub), publisher(NULL) {
            reset(pub);
        }

        subscription_t(subscription_t &&movee)
            : intrusive_list_node_t<subscription_t>(std::move(movee)),
              subscriber(std::move(movee.subscriber)),
              publisher(movee.publisher) {
            movee.publisher = nullptr;
        }

        /* Cause us to be subscribed to the given publisher (if any) and not to
        any other publisher. */
        void reset(publisher_t *pub = nullptr) {
            if (publisher) {
                DEBUG_VAR mutex_assertion_t::acq_t acq(&publisher->mutex);
                publisher->subscriptions.remove(this);
            }
            publisher = pub;
            if (publisher) {
                DEBUG_VAR mutex_assertion_t::acq_t acq(&publisher->mutex);
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
        friend class publisher_t<subscriber_t>;

        publisher_t *publisher;

        DISABLE_COPYING(subscription_t);
    };

    void rethread(threadnum_t new_thread) {
        rassert(subscriptions.empty(),
                "Cannot rethread a `publisher_t` that has subscribers.");
        mutex.rethread(new_thread);
    }

private:
    friend class subscription_t;
    friend class publisher_controller_t<subscriber_t>;

    publisher_t() { }
    ~publisher_t() { reset(); }

    void reset() {
        rassert(subscriptions.empty());
    }

    publisher_t(publisher_t &&movee)
        : subscriptions(std::move(movee.subscriptions)),
          mutex(std::move(movee.mutex)) {
        for (subscription_t *p = subscriptions.head(); p != nullptr;
             p = subscriptions.next(p)) {
            rassert(p->publisher == &movee);
            p->publisher = this;
        }
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
class publisher_controller_t {
public:
    publisher_controller_t() { }
    publisher_controller_t(publisher_controller_t &&movee)
        : publisher(std::move(movee.publisher)) { }

    publisher_t<subscriber_t> *get_publisher() {
        return &publisher;
    }

    template<class callable_t>
    void publish(const callable_t &callable) {
        DEBUG_VAR mutex_assertion_t::acq_t acq(&publisher.mutex);
        for (auto sub = publisher.subscriptions.head();
             sub != nullptr;
             sub = publisher.subscriptions.next(sub)) {
            /* `callable()` should not block */
            ASSERT_FINITE_CORO_WAITING;
            callable(sub->subscriber);
        }
    }

    void rethread(threadnum_t new_thread) {
        rassert(publisher.subscriptions.empty(),
                "Cannot rethread a `publisher_t` that has subscribers.");
        publisher.rethread(new_thread);
    }

    void reset() {
        publisher.reset();
    }

private:
    publisher_t<subscriber_t> publisher;
    DISABLE_COPYING(publisher_controller_t);
};

#endif /* CONCURRENCY_PUBSUB_HPP_ */
