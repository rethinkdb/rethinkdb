#ifndef __CONCURRENCY_GATE_HPP__
#define __CONCURRENCY_GATE_HPP__

#include "concurrency/resettable_cond_var.hpp"
#include "concurrency/pmap.hpp"
#include "utils2.hpp"

/* A gate is a concurrency object that monitors some shared resource. Opening the
gate allows actors to access the resource. Closing the gate prevents actors from
accessing the resource, but blocks until all the actors are done accessing the
resource.

gate_t is not thread-safe! If you want thread-safety, use threadsafe_gate_t. */

struct gate_t {

    /* Gate is closed in the constructor and must be closed in the destructor. */
    gate_t() : open(false), num_inside_gate(0) {
        pulsed_when_num_inside_gate_zero.pulse();
    }
    ~gate_t() {
        rassert(!open);
    }

    bool is_open() {
        return open;
    }

    /* Sentry that represents using the resource that the gate protects */
    struct entry_t : public home_thread_mixin_t {
        entry_t(gate_t *g) : gate(g) {
            rassert(gate->open);
            gate->num_inside_gate++;
            if (gate->num_inside_gate == 1) gate->pulsed_when_num_inside_gate_zero.reset();
        }
        ~entry_t() {
            if (gate->num_inside_gate == 1) gate->pulsed_when_num_inside_gate_zero.pulse();
            gate->num_inside_gate--;
        }
    private:
        DISABLE_COPYING(entry_t);
        gate_t *gate;
    };

    /* Sentry that opens and then closes gate */
    struct open_t : public home_thread_mixin_t {
        open_t(gate_t *g) : gate(g) {
            rassert(!gate->open);
            gate->open = true;
        }
        ~open_t() {
            rassert(gate->open);
            gate->open = false;
            gate->pulsed_when_num_inside_gate_zero.get_signal()->wait();
        }
    private:
        DISABLE_COPYING(open_t);
        gate_t *gate;
    };

private:
    friend class open_t;
    friend class entry_t;

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

    threadsafe_gate_t() :
        subgates(new gate_t*[get_num_threads()])
    {
        pmap(get_num_threads(), boost::bind(&threadsafe_gate_t::create_gate, this, _1));
    }

    ~threadsafe_gate_t() {
        pmap(get_num_threads(), boost::bind(&threadsafe_gate_t::destroy_gate, this, _1));
    }

    bool is_open() {
        return subgates[get_thread_id()]->is_open();
    }

    /* Sentry that represents using the resource that the gate protects */
    struct entry_t {
        entry_t(threadsafe_gate_t *g) : subentry(g->subgates[get_thread_id()]) { }
    private:
        DISABLE_COPYING(entry_t);
        gate_t::entry_t subentry;
    };

    /* Sentry that opens and then closes gate */
    struct open_t {
        open_t(threadsafe_gate_t *g) : gate(g) {
            subopens.resize(get_num_threads());
            pmap(get_num_threads(), boost::bind(&open_t::do_open, this, _1));
        }
        ~open_t() {
            pmap(get_num_threads(), boost::bind(&open_t::do_close, this, _1));
        }
    private:
        DISABLE_COPYING(open_t);
        void do_open(int thread) {
            on_thread_t thread_switcher(thread);
            subopens[thread] = new gate_t::open_t(gate->subgates[thread]);
        }
        void do_close(int thread) {
            on_thread_t thread_switcher(thread);
            delete subopens[thread];
        }
        threadsafe_gate_t *gate;
        std::vector<gate_t::open_t *> subopens;
    };

private:
    friend class open_t;
    friend class entry_t;
    void create_gate(int i) {
        on_thread_t thread_switcher(i);
        subgates[i] = new gate_t;
    }
    void destroy_gate(int i) {
        on_thread_t thread_switcher(i);
        delete subgates[i];
    }
    boost::scoped_array<gate_t*> subgates;
};

#endif /* __CONCURRENCY_GATE_HPP__ */
