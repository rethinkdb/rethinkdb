#include "concurrency/pubsub.hpp"

#include "arch/runtime/runtime.hpp"
#include "concurrency/mutex.hpp"

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
    mutex_acquisition_t acq(publisher->parent->mutex);
    publisher->parent->subscriptions.push_back(this);
}

template<class subscriber_t>
void publisher_t<subscriber_t>::subscription_t::unsubscribe() {
    assert_thread();
    rassert(publisher);
    mutex_acquisition_t acq(publisher->parent->mutex);
    publisher->parent->subscriptions.remove(this);
    publisher = NULL;
}

template<class subscriber_t>
publisher_t<subscriber_t>::subscription_t::~subscription_t() {
    if (publisher) unsubscribe();
}

template<class subscriber_t>
template<class callable_t>
void publisher_controller_t<subscriber_t>::publish(const callable_t &callable, mutex_acquisition_t *proof) {
    proof->assert_is_holding(mutex);
    mutex_acquisition_t temp;
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

template class publisher_t<boost::function<void()> >;
template class publisher_controller_t<boost::function<void()> >;

template void publisher_controller_t<boost::function<void ()> >::publish<void (*)(boost::function<void ()>&)>(void (* const&)(boost::function<void ()>&), mutex_acquisition_t*);
