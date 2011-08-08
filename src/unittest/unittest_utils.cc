#include "unittest/unittest_utils.hpp"

#include "arch/arch.hpp"

struct starter_t : public thread_message_t {
    thread_pool_t *tp;
    boost::function<void()> fun;
    starter_t(thread_pool_t *tp, boost::function<void()> fun) : tp(tp), fun(fun) { }
    void run() {
        fun();
        tp->shutdown();
    }
    void on_thread_switch() {
        coro_t::spawn_now(boost::bind(&starter_t::run, this));
    }
};

void run_in_thread_pool(boost::function<void()> fun) {
    thread_pool_t thread_pool(1);
    starter_t starter(&thread_pool, fun);
    thread_pool.run(&starter);
}