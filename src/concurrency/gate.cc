#include "concurrency/gate.hpp"

#include <boost/bind.hpp>

#include "arch/runtime/runtime.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/signal.hpp"

gate_t::gate_t() : open(false), num_inside_gate(0) {
    pulsed_when_num_inside_gate_zero.pulse();
}

gate_t::~gate_t() {
    rassert(!open);
}

bool gate_t::is_open() {
    return open;
}

gate_t::entry_t::entry_t(gate_t *g) : gate(g) {
    rassert(gate->open);
    gate->num_inside_gate++;
    if (gate->num_inside_gate == 1) {
        gate->pulsed_when_num_inside_gate_zero.reset();
    }
}

gate_t::entry_t::~entry_t() {
    if (gate->num_inside_gate == 1) {
        gate->pulsed_when_num_inside_gate_zero.pulse();
    }
    gate->num_inside_gate--;
}

gate_t::open_t::open_t(gate_t *g) : gate(g) {
    rassert(!gate->open);
    gate->open = true;
}

gate_t::open_t::~open_t() {
    rassert(gate->open);
    gate->open = false;
    gate->pulsed_when_num_inside_gate_zero.get_signal()->wait();
}





threadsafe_gate_t::threadsafe_gate_t()
    : subgates(new gate_t*[get_num_threads()])
{
    pmap(get_num_threads(), boost::bind(&threadsafe_gate_t::create_gate, this, _1));
}

threadsafe_gate_t::~threadsafe_gate_t() {
    pmap(get_num_threads(), boost::bind(&threadsafe_gate_t::destroy_gate, this, _1));
}

bool threadsafe_gate_t::is_open() {
    return subgates[get_thread_id()]->is_open();
}

threadsafe_gate_t::entry_t::entry_t(threadsafe_gate_t *g)
    : subentry(g->subgates[get_thread_id()]) { }

threadsafe_gate_t::open_t::open_t(threadsafe_gate_t *g) : gate(g) {
    subopens.resize(get_num_threads());
    pmap(get_num_threads(), boost::bind(&open_t::do_open, this, _1));
}

threadsafe_gate_t::open_t::~open_t() {
    pmap(get_num_threads(), boost::bind(&open_t::do_close, this, _1));
}

void threadsafe_gate_t::open_t::do_open(int thread) {
    on_thread_t thread_switcher(thread);
    subopens[thread] = new gate_t::open_t(gate->subgates[thread]);
}

void threadsafe_gate_t::open_t::do_close(int thread) {
    on_thread_t thread_switcher(thread);
    delete subopens[thread];
}

void threadsafe_gate_t::create_gate(int i) {
    on_thread_t thread_switcher(i);
    subgates[i] = new gate_t;
}

void threadsafe_gate_t::destroy_gate(int i) {
    on_thread_t thread_switcher(i);
    delete subgates[i];
}
