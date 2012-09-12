#ifndef CONCURRENCY_AUTO_DRAINER_HPP_
#define CONCURRENCY_AUTO_DRAINER_HPP_

#include "concurrency/cond_var.hpp"

/* `auto_drainer_t` is useful when you have a resource and some number of
processes that use that resource, and you need to be sure that all the processes
are done before the resource is destroyed.

Construct an `auto_drainer_t` for the resource. Have each process construct an
`auto_drainer_t::lock_t` and hold it. When the resource must be destroyed, first
prevent any new processes from starting and then call the `auto_drainer_t`'s
destructor. The destructor will block until all the `lock_t` objects have been
destroyed.

The signal returned by `lock_t::get_drain_signal()` will be pulsed when the
`auto_drainer_t`'s destructor is called. You can use this to interrupt the
running processes if it makes sense for your situation. */

class auto_drainer_t : public home_thread_mixin_t {
public:
    auto_drainer_t();
    ~auto_drainer_t();

    class lock_t {
    public:
        lock_t();
        explicit lock_t(auto_drainer_t *);
        lock_t(const lock_t &);
        lock_t &operator=(const lock_t &);
        void reset();
        signal_t *get_drain_signal() const;
        void assert_is_holding(auto_drainer_t *);
        ~lock_t();
    private:
        auto_drainer_t *parent;
    };

    void assert_not_draining() {
        rassert(!draining.is_pulsed());
    }

    void rethread(int new_thread) {
        rassert(refcount == 0);
        real_home_thread = new_thread;
        draining.rethread(new_thread);
    }

private:
    void incref();
    void decref();

    cond_t draining;
    int refcount;
    coro_t *when_done;
};

/* For example, here's a toy class to handle TCP connections:

    class toy_tcp_server_t {

    public:
        toy_tcp_server_t(int port) :
            listener(port,
                boost::bind(&toy_tcp_server_t::serve,
                    this,
                    auto_drainer_t::lock_t(&drainer),
                    _1
                    )
                )
            { }

    private:
        void serve(auto_drainer_t::lock_t keepalive, scoped_ptr_t<tcp_conn_t> &conn) {
            signal_t *interruptor = keepalive.get_drain_signal();
            try {
                serve_queries_until_interrupted(conn, interruptor);
            } catch (interrupted_exc_t) {
                // Ignore
            }
        }

        auto_drainer_t drainer;
        tcp_listener_t listener;
    };

When a TCP connection comes in, the functor created by `boost::bind()` is
called, and it makes a copy of the `auto_drainer_t::lock_t` and passes it to
`serve()`. When the `toy_tcp_server_t` is destroyed, first the `tcp_listener_t`
destructor is run (so no new connections are accepted) and then the
`auto_drainer_t` destructor is run. `keepalive.get_drain_signal()` is pulsed,
which interrupts `serve_queries_until_interrupted()`, causing `serve()` to
return and destroy `keepalive`. This way, `drainer`'s destructor blocks until
all the copies of `serve` have returned. */

#endif /* CONCURRENCY_AUTO_DRAINER_HPP_ */
