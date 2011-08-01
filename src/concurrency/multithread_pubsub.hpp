#ifndef __CONCURRENCY_MULTITHREAD_PUBSUB_HPP__
#define __CONCURRENCY_MULTITHREAD_PUBSUB_HPP__

#include "concurrency/pubsub.hpp"
#include <boost/scoped_array.hpp>
#include <boost/bind.hpp>
#include "concurrency/pmap.hpp"

/* `multithread_publisher_t` publishes to subscribers on any thread, not just on
the same thread as the publisher. No ordering guarantees are provided between
different publications published on different threads. */

template<class subscriber_t>
class multithread_publisher_t {

public:
    class subscription_t :
        public home_thread_mixin_t
    {
        subscription_t(subscriber_t sub) :
            subs(sub) { }
        subscription_t(subscriber_t sub, multithread_publisher_t *pub) :
            subs(sub, &pub->children[get_thread_id()]) { }
        void resubscribe(multithread_publisher_t *pub) {
            subs.resubscribe(&pub->children[get_thread_id()]);
        }
        void unsubscribe() {
            subs.unsubscribe();
        }
    private:
        typename publisher_t<subscriber_t>::subscription_t subs;
    };

protected:
    multithread_publisher_t() :
        children(new publisher_t<subscriber_t>[get_num_threads()]) { }

    template<class callable_t>
    void publish(const callable_t &callable) {
        pmap(get_num_threads, boost::bind(
            &multithread_publisher_t<subscriber_t>::publish_on_thread<callable_t>, this,
            _1, &callable));
    }

private:
    friend class subscription_t;
    template<class callable_t>
    void publish_on_thread(int thread, const callable_t *callable) {
        on_thread_t thread_switcher(thread);
        children[thread].publish(*callable);
    }
    boost::scoped_array<publisher_t<subscriber_t> > children;
};

#endif /* __CONCURRENCY_MULTITHREAD_PUBSUB_HPP__ */
