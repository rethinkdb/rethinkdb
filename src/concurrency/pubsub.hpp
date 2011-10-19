#ifndef __CONCURRENCY_PUBSUB_HPP__
#define __CONCURRENCY_PUBSUB_HPP__

#include "containers/intrusive_list.hpp"
#include "utils.hpp"

class mutex_t;
class mutex_acquisition_t;

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
    explicit publisher_controller_t(mutex_t *m) :
        publisher(this),
        mutex(m),
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
    void publish(const callable_t &callable, mutex_acquisition_t *proof);

    void rethread(int new_thread) {

        rassert(subscriptions.empty(), "Cannot rethread a `publisher_t` that "
            "has subscribers.");

        real_home_thread = new_thread;
        publisher.real_home_thread = new_thread;
    }

private:
    friend class publisher_t<subscriber_t>::subscription_t;

    publisher_t<subscriber_t> publisher;

    intrusive_list_t<typename publisher_t<subscriber_t>::subscription_t> subscriptions;

    mutex_t *mutex;
    bool publishing;

    DISABLE_COPYING(publisher_controller_t);
};

#endif /* __CONCURRENCY_PUBSUB_HPP__ */
