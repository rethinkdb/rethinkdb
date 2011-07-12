#ifndef __CONCURRENCY_GATE_HPP__
#define __CONCURRENCY_GATE_HPP__

#include <vector>

#include "errors.hpp"
#include <boost/scoped_array.hpp>

#include "concurrency/resettable_cond_var.hpp"
#include "utils.hpp"

/* A gate is a concurrency object that monitors some shared resource. Opening the
gate allows actors to access the resource. Closing the gate prevents actors from
accessing the resource, but blocks until all the actors are done accessing the
resource.

gate_t is not thread-safe! If you want thread-safety, use threadsafe_gate_t. */

struct gate_t {

    /* Gate is closed in the constructor and must be closed in the destructor. */
    gate_t();
    ~gate_t();

    bool is_open();

    /* Sentry that represents using the resource that the gate protects */
    struct entry_t : public home_thread_mixin_t {
        entry_t(gate_t *g);
        ~entry_t();
    private:
        DISABLE_COPYING(entry_t);
        gate_t *gate;
    };

    /* Sentry that opens and then closes gate */
    struct open_t : public home_thread_mixin_t {
        open_t(gate_t *g);
        ~open_t();
    private:
        DISABLE_COPYING(open_t);
        gate_t *gate;
    };

private:
    friend struct open_t;
    friend struct entry_t;

    /* True if the gate is open. False if the gate is closed or in the process of closing. */
    bool open;

    /* The number of actors currently inside the gate. Can be >0 even when 'open' is false
    if the gate is in the process of closing. */
    int num_inside_gate;

    resettable_cond_t pulsed_when_num_inside_gate_zero;
};

/* threadsafe_gate_t is a thread-safe version of gate_t. It can be entered on any thread
and opened/closed from any thread. Its behavior is undefined if you open/close it on two
threads concurrently. */

struct threadsafe_gate_t {
    threadsafe_gate_t();
    ~threadsafe_gate_t();
    bool is_open();

    /* Sentry that represents using the resource that the gate protects */
    struct entry_t {
        entry_t(threadsafe_gate_t *g);
    private:
        gate_t::entry_t subentry;

        DISABLE_COPYING(entry_t);
    };

    /* Sentry that opens and then closes gate */
    struct open_t {
        open_t(threadsafe_gate_t *g);
        ~open_t();
    private:
        void do_open(int thread);
        void do_close(int thread);
        threadsafe_gate_t *gate;
        std::vector<gate_t::open_t *> subopens;

        DISABLE_COPYING(open_t);
    };

private:
    friend struct open_t;
    friend struct entry_t;
    void create_gate(int i);
    void destroy_gate(int i);
    boost::scoped_array<gate_t*> subgates;
};

#endif /* __CONCURRENCY_GATE_HPP__ */
