#include "conn_acceptor.hpp"
#include "db_thread_info.hpp"

conn_acceptor_t::conn_acceptor_t(int port, conn_handler_t *(*creator)(net_conn_t*, void*), void *udata)
    : state(state_off), port(port), creator(creator), creator_udata(udata), listener(NULL), next_thread(0), n_active_conns(0) { }

conn_acceptor_t::~conn_acceptor_t() {
    assert(state == state_off);
    assert(!listener);
    assert(n_active_conns == 0);
}

bool conn_acceptor_t::start() {
    listener = new net_listener_t(port);
    
    listener->set_callback(this);

    state = state_ready;
    
    // Check for error conditions:
    return !listener->defunct;
}

void conn_acceptor_t::on_net_listener_accept(net_conn_t *conn) {
    assert(state == state_ready);

    n_active_conns++;
    int thread = next_thread++ % get_num_db_threads();
    do_on_thread(thread, this, &conn_acceptor_t::create_conn_on_this_core, conn);
}

bool conn_acceptor_t::create_conn_on_this_core(net_conn_t *conn) {
    conn_handler_t *handler = creator(conn, creator_udata);
    assert(!handler->parent);
    handler->parent = this;
    handler->conn = conn;
    conn_handlers[get_thread_id()].push_back(handler);

    return true;
}

bool conn_acceptor_t::have_shutdown_a_conn() {
    assert(state == state_ready || state == state_shutting_down);

    n_active_conns--;
    assert(n_active_conns >= 0);

    if (state == state_shutting_down && n_active_conns == 0) {
        state = state_off;
        if (shutdown_callback) shutdown_callback->on_conn_acceptor_shutdown();
    }

    return true;
}

bool conn_acceptor_t::shutdown(shutdown_callback_t *cb) {
    assert(state == state_ready);

    /* Stop accepting new network connections */

    delete listener;
    listener = NULL;

    /* Notify any existing network connections to shut down */

    for (int i = 0; i < get_num_db_threads(); i++) {
        do_on_thread(i, this, &conn_acceptor_t::shutdown_conns_on_this_core);
    }

    /* Check if we can proceed immediately */

    if (n_active_conns > 0) {
        state = state_shutting_down;
        shutdown_callback = cb;
        return false;
    } else {
        state = state_off;
        return true;
    }
}

bool conn_acceptor_t::shutdown_conns_on_this_core() {
    while (conn_handler_t *h = conn_handlers[get_thread_id()].head()) {
        assert(!h->quitting);   // If it's already quitting it shouldn't be in the list
        h->quit();
        
        /* quit() should have called on_quit() which should have removed it from the list */
        assert(h != conn_handlers[get_thread_id()].head());
    }

    return true;
}

perfmon_counter_t pm_conns_total("conns_total[conns]");

conn_handler_t::conn_handler_t() : parent(NULL), quitting(false) {
    pm_conns_total++;
}

void conn_handler_t::on_quit() {
    assert(parent);   // We shouldn't get an on_quit() before we finished starting

    assert(!quitting);
    quitting = true;

    /* Remove ourselves from the conn_handlers list immediately so that if the server
    gets a SIGINT before we get an on_conn_fsm_shutdown(), the conn_fsm_t won't be
    told to quit when it already is quitting. */
    parent->conn_handlers[get_thread_id()].remove(this);
}

conn_handler_t::~conn_handler_t() {
    assert(parent);
    assert(quitting);
    do_on_thread(parent->home_thread, parent, &conn_acceptor_t::have_shutdown_a_conn);
    pm_conns_total--;
    delete conn;
}
