#ifndef __CONCURRENCY_PUBSUB_HPP__
#define __CONCURRENCY_PUBSUB_HPP__

#include "concurrency/rwi_lock.hpp"
#include "utils.hpp"
#include "arch/runtime/runtime.hpp"

/* Forward declaration */

template<class subscriber_t>
class publisher_controller_t;

/* `publisher_t` is used to implement small-scale pub/sub architectures. It's
useful whenever you need to broadcast notifications to some number of listeners.

Each publisher has zero or more subscriptions. Each subscription is associated
with a subscriber. A subscription is usually associated with one publisher, but
it is possible to unassociate a subscription or to move it from one publisher to
another. */

template<class subscriber_t>
class publisher_t :
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
        explicit subscription_t(subscriber_t sub);

        /* Construct a `subscription_t` and subscribe to the given publisher. */
        subscription_t(subscriber_t sub, publisher_t *pub);

        /* Subscribe to the given publisher. It's only legal to call
        `resubscribe()` if you are not subscribed to a publisher. */
        void resubscribe(publisher_t *pub);

        /* Unsubscribe from the publisher you are subscribed to. */
        void unsubscribe();

        /* Unsubscribe from the publisher you are subscribed to, if there is
        such a publisher. */
        ~subscription_t();

    private:
        friend class publisher_controller_t<subscriber_t>;

        subscriber_t subscriber;
        publisher_t *publisher;

        DISABLE_COPYING(subscription_t);
    };

private:
    friend class subscription_t;
    friend class publisher_controller_t<subscriber_t>;

    publisher_t(publisher_controller_t<subscriber_t> *p, int specified_home_thread)
        : home_thread_mixin_t(specified_home_thread), parent(p) { }
    explicit publisher_t(publisher_controller_t<subscriber_t> *p) : parent(p) { }

    publisher_controller_t<subscriber_t> *parent;

    DISABLE_COPYING(publisher_t);
};

/* To create a publisher, create a `publisher_controller_t` and call its
`get_publisher()` method. `publisher_t` is split from `publisher_controller_t`
so that you can pass a `publisher_t *` to things; they will be able to listen
for events, but not publish events themselves.

To publish things, call `publish()` on the `publisher_controller_t` you
created. `publish()` takes a function, which will be called once for each
subscription. The function must not block.

When you construct a `publisher_controller_t`, you must pass the address of a
`mutex_t` you created. You must acquire this mutex before you call `publish()`.
If you try to subscribe or unsubscribe while the mutex is acquired, it blocks.
The purpose of having a mutex is to prevent races between publishing things and
adding/removing subscribers. */

template<class subscriber_t>
struct publisher_controller_t :
    public home_thread_mixin_t
{
public:
    publisher_controller_t(rwi_lock_t *l, int specified_home_thread)
        : home_thread_mixin_t(specified_home_thread),
          publisher(this, specified_home_thread),
          lock(l),
          publishing(false) { }

    explicit publisher_controller_t(rwi_lock_t *l) :
        publisher(this),
        lock(l),
        publishing(false) { }

    ~publisher_controller_t() {
        /* User is responsible for unsubscribing everything before destroying
        the publisher. */
        rassert(subscriptions.empty());

        /* We shouldn't be destroyed from within a publisher callback; that's
        dangerous. */
        rassert(!publishing);
    }

    publisher_t<subscriber_t> *get_publisher() {
        return &publisher;
    }

    template<class callable_t>
    void publish(const callable_t &callable, rwi_lock_t::write_acq_t *proof) {

        proof->assert_is_holding(lock);
        rwi_lock_t::write_acq_t temp;
        swap(*proof, temp);

        rassert(!publishing);
        publishing = true;

        for (typename publisher_t<subscriber_t>::subscription_t *sub = subscriptions.head();
             sub;
             sub = subscriptions.next(sub)) {
            /* `callable()` should not block */
            ASSERT_FINITE_CORO_WAITING;
            callable(sub->subscriber);
        }

        rassert(publishing);
        publishing = false;

        swap(*proof, temp);
    }

private:
    friend class publisher_t<subscriber_t>::subscription_t;

    publisher_t<subscriber_t> publisher;

    intrusive_list_t<typename publisher_t<subscriber_t>::subscription_t> subscriptions;

    rwi_lock_t *lock;
    bool publishing;

    DISABLE_COPYING(publisher_controller_t);
};

/* `publisher_t<subscriber_t>::subscription_t` is implemented down here because
it needs to know about `publisher_controller_t`. */

template<class subscriber_t>
publisher_t<subscriber_t>::subscription_t::subscription_t(subscriber_t sub) :
    subscriber(sub), publisher(NULL) { }

template<class subscriber_t>
publisher_t<subscriber_t>::subscription_t::subscription_t(subscriber_t sub, publisher_t *pub) :
    subscriber(sub), publisher(NULL) {
    resubscribe(pub);
}

template<class subscriber_t>
void publisher_t<subscriber_t>::subscription_t::resubscribe(publisher_t *pub) {
    assert_thread();
    rassert(!publisher);
    rassert(pub);
    pub->assert_thread();
    publisher = pub;
    rwi_lock_t::read_acq_t acq(publisher->parent->lock);
    publisher->parent->subscriptions.push_back(this);
}

template<class subscriber_t>
void publisher_t<subscriber_t>::subscription_t::unsubscribe() {
    assert_thread();
    rassert(publisher);
    rwi_lock_t::read_acq_t acq(publisher->parent->lock);
    publisher->parent->subscriptions.remove(this);
    publisher = NULL;
}

template<class subscriber_t>
publisher_t<subscriber_t>::subscription_t::~subscription_t() {
    if (publisher) unsubscribe();
}

#endif /* __CONCURRENCY_PUBSUB_HPP__ */
