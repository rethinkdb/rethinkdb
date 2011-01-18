#include "failover.hpp"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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
        execv(script_path, NULL); //This works right @jdoliner
        exit(0);
    }
}

/* Failover_t implementation */
void Failover_t::add_callback(failover_callback_t *cb) {
    callbacks.push_back(cb);
}

void Failover_t::on_failure() {
    for (intrusive_list_t<failover_callback_t>::iterator it = callbacks.begin(); it != callbacks.end(); it++)
        (*it).on_failure();
}
