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
failover_script_callback_t::failover_script_callback_t(const char *script_path) 
    :script_path(strdup(script_path))
{}

failover_script_callback_t::~failover_script_callback_t() {
    delete script_path;
}

void failover_script_callback_t::on_failure() {
    pid_t pid = fork();

    if (pid == 0) {
        execl(script_path, script_path, "down", NULL); //This works right @jdoliner, yes joe from 2 weeks ago it does - jdoliner
        exit(0);
    }
}

void failover_script_callback_t::on_resume() {
     pid_t pid = fork();

    if (pid == 0) {
        execl(script_path, script_path, "up", NULL); //This works right @jdoliner
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
