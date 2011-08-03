#ifndef __CONCURRENCY_PUBSUB_HPP__
#define __CONCURRENCY_PUBSUB_HPP__

#include "concurrency/mutex.hpp"
#include "utils.hpp"
#include "arch/runtime/coroutines.hpp"

/* `publisher_t` is used to implement small-scale pub/sub architectures. It's
useful whenever you need to broadcast notifications to some number of listeners.

Each publisher has zero or more subscriptions. Each subscription is associated
with a subscriber. A subscription is usually associated with one publisher, but
it is possible to unassociate a subscription or to move it from one publisher to
another.

Broadcast a message to subscribers using `publisher_t::publish()`. `publish()`
takes a function, which will be called once for each subscription. The function
must not block. If you try to add or remove a subscription or delete the
publisher from inside the `publish()` callback, your call will block until
`publish()` is done. */

template<class subscriber_t>
struct publisher_t :
    public home_thread_mixin_t
{
public:
    class subscription_t :
        public intrusive_list_node_t<subscription_t>,
        public home_thread_mixin_t
    {
    public:
        /* Construct a `subscription_t` that is not subscribed to any publisher.
        */
        subscription_t(subscriber_t sub) :
            subscriber(sub), publisher(NULL) { }

        /* Construct a `subscription_t` and subscribe to the given publisher. */
        subscription_t(subscriber_t sub, publisher_t *pub) :
            subscriber(sub), publisher(NULL) {
            resubscribe(pub);
        }

        /* Subscribe to the given publisher. It's only legal to call
        `resubscribe()` if you are not subscribed to a publisher. */
        void resubscribe(publisher_t *pub) {
            assert_thread();
            rassert(!publisher);
            rassert(pub);
            pub->assert_thread();
            publisher = pub;
            rassert(!publisher->shutting_down);
            mutex_acquisition_t acq(&publisher->lock);
            publisher->subscriptions.push_back(this);
        }

        /* Unsubscribe from the publisher you are subscribed to. */
        void unsubscribe() {
            assert_thread();
            rassert(publisher);
            rassert(!publisher->shutting_down);
            mutex_acquisition_t acq(&publisher->lock);
            publisher->subscriptions.remove(this);
            publisher = NULL;
        }

        ~subscription_t() {
            if (publisher) unsubscribe();
        }

    private:
        friend class publisher_t;

        subscriber_t subscriber;
        publisher_t *publisher;

        DISABLE_COPYING(subscription_t);
    };

    void rethread(int new_thread) {

        rassert(subscriptions.empty(), "Cannot rethread a `publisher_t` that "
            "has subscribers.");

        real_home_thread = new_thread;
    }

protected:
    publisher_t() {
        DEBUG_ONLY(shutting_down = false);
    }

    ~publisher_t() {
        /* User is responsible for unsubscribing everything before destroying
        the publisher. */
        rassert(subscriptions.empty());

        /* Set `shutting_down` to `true` so that nothing gets in line behind us
        for the mutex, only to be confused when we destroy the mutex. */
        DEBUG_ONLY(shutting_down = true);

        /* Lock the mutex so that if we're being run from within a call to
        `publish()`, we won't destroy the `publisher_t` until the call to
        `publish` is done.
        
        TODO: This is terrible and the only reason we have it is because of
        `notify_now()`. `notify_now()` should go away. */
        co_lock_mutex(&lock);
    }

    template<class callable_t>
    void publish(const callable_t &callable) {
        rassert(!shutting_down);

        /* Acquire the mutex so that attempts to add new subscriptions, publish
        other events, or destroy the publisher will block until we're done. */
        mutex_acquisition_t acq(&lock, true);

        /* Broadcast to each subscription */
        for (subscription_t *sub = subscriptions.head(); sub;
             sub = subscriptions.next(sub)) {
            /* `callable()` should not block */
            ASSERT_FINITE_CORO_WAITING;
            callable(sub->subscriber);
        }

        /* `acq` destructor called here. After `acq` destructor returns, we
        may not access any local variables because the `publisher_t` may have
        been destroyed by something that took the mutex after us. */
    }

private:
    friend class subscription_t;
#ifndef NDEBUG
    bool shutting_down;
#endif
    mutex_t lock;
    intrusive_list_t<subscription_t> subscriptions;
    DISABLE_COPYING(publisher_t);
};

/* One way to use `publisher_t` is to construct a concrete subclass of
`publisher_t` but then pass around a pointer to the underlying `publisher_t`.
Things that have the pointer to the `publisher_t` can subscribe to receive
notifications, but cannot publish. `public_publisher_t` is a concrete subclass
that makes `publish()` and the constructor and destructor public. */

template<class subscriber_t>
struct public_publisher_t :
    public publisher_t<subscriber_t>
{
public:
    public_publisher_t() { }
    ~public_publisher_t() { }

    template<class callable_t>
    void publish(const callable_t &callable) {
         publisher_t<subscriber_t>::publish(callable);
    }

private:
    DISABLE_COPYING(public_publisher_t);
};

#endif /* __CONCURRENCY_PUBSUB_HPP__ */
