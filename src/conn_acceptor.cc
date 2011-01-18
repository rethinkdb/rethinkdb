#include "conn_acceptor.hpp"
#include "db_thread_info.hpp"
#include "concurrency/pmap.hpp"
#include "perfmon.hpp"

conn_acceptor_t::conn_acceptor_t(int port, handler_t *handler)
    : handler(handler), listener(new tcp_listener_t(port)), next_thread(0)
{
    listener->set_callback(this);
    if (listener->defunct) throw address_in_use_exc_t();
}

void conn_acceptor_t::on_tcp_listener_accept(tcp_conn_t *conn) {

    new conn_agent_t(this, conn);
}

conn_acceptor_t::conn_agent_t::conn_agent_t(conn_acceptor_t *parent, tcp_conn_t *conn)
    : parent(parent), conn(conn)
{
    coro_t::spawn(&conn_agent_t::run, this);
}

perfmon_duration_sampler_t pm_conns("conns", secs_to_ticks(600));

void conn_acceptor_t::conn_agent_t::run() {

    block_pm_duration conn_timer(&pm_conns);

    int thread = parent->next_thread++ % get_num_db_threads();
    {
        on_thread_t thread_switcher(thread);
    
        /* Lock the shutdown_lock so the parent can't shut stuff down without our connection closing
        first. Put ourselves in the conn_agents list so it can come close our connection if it needs
        to. */
        parent->shutdown_locks[thread].co_lock(rwi_read);
        parent->conn_agents[thread].push_back(this);
    
        parent->handler->handle(conn);
        if (conn->is_read_open()) conn->shutdown_read();
        if (conn->is_write_open()) conn->shutdown_write();
    
        /* Now release the lock and stuff because it's now OK for the parent to shut down. */
        parent->conn_agents[thread].remove(this);
        parent->shutdown_locks[thread].unlock();
    }

    delete conn;
    delete this;
}

conn_acceptor_t::~conn_acceptor_t() {

    listener.reset();   // Stop accepting any more new connections

    pmap(get_num_db_threads(), &conn_acceptor_t::close_connections, this);
}

void conn_acceptor_t::close_connections(int thread) {
    on_thread_t thread_switcher(thread);
    for (conn_agent_t *a = conn_agents[thread].head(); a; a = conn_agents[thread].next(a)) {
        if (a->conn->is_read_open()) a->conn->shutdown_read();
    }
    shutdown_locks[thread].co_lock(rwi_write);
}
