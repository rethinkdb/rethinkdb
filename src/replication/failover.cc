#include "failover.hpp"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* failover_callback_t implementation */
failover_callback_t::failover_callback_t() 
    : parent(NULL)
{}
failover_callback_t::~failover_callback_t() {
    if (parent)
        parent->remove_callback(this);
}

/* failover script implementation */
failover_script_callback_t::failover_script_callback_t(const char *_script_path) 
    :script_path(_script_path)
{}

failover_script_callback_t::~failover_script_callback_t() { }

void failover_script_callback_t::on_failure() {
    pid_t pid = fork();

    if (pid == 0) {
        execl(script_path.c_str(), script_path.c_str(), "down", NULL); //This works right @jdoliner, yes joe from 2 weeks ago it does - jdoliner
        exit(0);
    }
}

void failover_script_callback_t::on_resume() {
     pid_t pid = fork();

    if (pid == 0) {
        execl(script_path.c_str(), script_path.c_str(), "up", NULL); //This works right @jdoliner
        exit(0);
    }
}

/* Failover_t implementation */
failover_t::failover_t() {}

failover_t::~failover_t() {
    on_thread_t thread_switch(home_thread);
    while (callbacks.head()) {
        callbacks.head()->parent = NULL;
        callbacks.remove(callbacks.head());
    }
}

void failover_t::add_callback(failover_callback_t *cb) {
    on_thread_t thread_switch(home_thread);
    callbacks.push_back(cb);
    cb->parent = this;
}

void failover_t::remove_callback(failover_callback_t *cb) {
    on_thread_t thread_switch(home_thread);
    callbacks.remove(cb);
    cb->parent = NULL;
}

void failover_t::on_failure() {
    for (intrusive_list_t<failover_callback_t>::iterator it = callbacks.begin(); it != callbacks.end(); it++)
        (*it).on_failure();
}

void failover_t::on_resume() {
    for (intrusive_list_t<failover_callback_t>::iterator it = callbacks.begin(); it != callbacks.end(); it++)
        (*it).on_resume();
}

void failover_t::on_backfill_begin() {
    for (intrusive_list_t<failover_callback_t>::iterator it = callbacks.begin(); it != callbacks.end(); it++)
        (*it).on_backfill_begin();
}

void failover_t::on_backfill_end() {
    for (intrusive_list_t<failover_callback_t>::iterator it = callbacks.begin(); it != callbacks.end(); it++)
        (*it).on_backfill_end();
}

void failover_t::on_reverse_backfill_begin() {
    for (intrusive_list_t<failover_callback_t>::iterator it = callbacks.begin(); it != callbacks.end(); it++)
        (*it).on_reverse_backfill_begin();
}

void failover_t::on_reverse_backfill_end() {
    for (intrusive_list_t<failover_callback_t>::iterator it = callbacks.begin(); it != callbacks.end(); it++)
        (*it).on_reverse_backfill_end();
}

/* failover_query_enabler_disabler_t is responsible for deciding when to let sets and
gets in and when not to. */

failover_query_enabler_disabler_t::failover_query_enabler_disabler_t(gated_set_store_interface_t *sg, gated_get_store_t *gg)
    : set_gate(sg), get_gate(gg)
{
    /* Initially, allow gets but not sets */
    permit_gets.reset(new gated_get_store_t::open_t(get_gate));
    set_gate->set_message("can't run sets against this server; we're still trying to reach master");
}

void failover_query_enabler_disabler_t::on_failure() {
    /* When master fails, start permitting sets. */
    permit_sets.reset(new gated_set_store_interface_t::open_t(set_gate));
}

void failover_query_enabler_disabler_t::on_resume() {
    /* When master comes up, stop permitting sets. */
    permit_sets.reset();
    set_gate->set_message("illegal to run sets against a slave while master is up");
}

void failover_query_enabler_disabler_t::on_backfill_begin() {
    /* During backfilling, don't allow gets */
    permit_gets.reset();
    get_gate->set_message("we're in the middle of a backfill; the database state is inconsistent.");
}

void failover_query_enabler_disabler_t::on_backfill_end() {
    /* Now that backfill is over, allow gets */
    permit_gets.reset(new gated_get_store_t::open_t(get_gate));
}

