#ifndef __CONCURRENCY_GATE_HPP__
#define __CONCURRENCY_GATE_HPP__

#include "concurrency/cond_var.hpp"

/* A gate is a concurrency object that monitors some shared resource. Opening the
gate allows actors to access the resource. Closing the gate prevents actors from
accessing the resource, but blocks until all the actors are done accessing the
resource. */

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
    struct entry_t {
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
        gate_t *gate;
    };

    /* Sentry that opens and then closes gate */
    struct open_t {
        open_t(gate_t *g) : gate(g) {
            rassert(!gate->open);
            gate->open = true;
        }
        ~open_t() {
            rassert(gate->open);
            gate->open = false;
            gate->pulsed_when_num_inside_gate_zero.wait();
        }
    private:
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

    cond_t pulsed_when_num_inside_gate_zero;
};

#endif /* __CONCURRENCY_GATE_HPP__ */
