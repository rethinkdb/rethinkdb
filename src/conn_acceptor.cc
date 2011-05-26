#include "conn_acceptor.hpp"
#include "db_thread_info.hpp"
#include "concurrency/pmap.hpp"
#include "perfmon.hpp"

conn_acceptor_t::conn_acceptor_t(int port, conn_acceptor_callback_t *_acceptor_callback) :
    acceptor_callback(_acceptor_callback),
    listener(new tcp_listener_t(port, boost::bind(&conn_acceptor_t::on_conn, this, _1))),
    next_thread(0)
    { }

void conn_acceptor_t::on_conn(boost::scoped_ptr<tcp_conn_t>& conn) {

    conn_agent_t agent(this, conn.get());
    agent.run();
}

conn_acceptor_t::conn_agent_t::conn_agent_t(conn_acceptor_t *parent, tcp_conn_t *conn)
    : parent(parent), conn(conn)
{
}

perfmon_duration_sampler_t pm_conns("conns", secs_to_ticks(600), false);

void conn_acceptor_t::conn_agent_t::run() {

    block_pm_duration conn_timer(&pm_conns);

    int thread = parent->next_thread++ % get_num_db_threads();
    {
        boost::scoped_ptr<conn_handler_with_special_lifetime_t> handler;
        parent->acceptor_callback->make_handler_for_conn_thread(handler);

        home_thread_mixin_t::rethread_t unregister_conn(conn, INVALID_THREAD);
        on_thread_t thread_switcher(thread);
        home_thread_mixin_t::rethread_t reregister_conn(conn, get_thread_id());

        /* Lock the shutdown_lock so the parent can't shut stuff down without our connection closing
        first. Put ourselves in the conn_agents list so it can come close our connection if it needs
        to. */
        parent->shutdown_locks[thread].co_lock(rwi_read);
        parent->conn_agents[thread].push_back(this);

        handler->talk_on_connection(conn);
        if (conn->is_read_open()) conn->shutdown_read();
        if (conn->is_write_open()) conn->shutdown_write();

        /* Now release the lock and stuff because it's now OK for the parent to shut down. */
        parent->conn_agents[thread].remove(this);
        parent->shutdown_locks[thread].unlock();
    }
}

conn_acceptor_t::~conn_acceptor_t() {
    on_thread_t thread_switcher(home_thread); //make sure the listener gets deregistered on the right thread

    listener.reset();   // Stop accepting any more new connections

    pmap(get_num_db_threads(), boost::bind(&conn_acceptor_t::close_connections, this, _1));
}

void conn_acceptor_t::close_connections(int thread) {
    on_thread_t thread_switcher(thread);
    for (conn_agent_t *a = conn_agents[thread].head(); a; a = conn_agents[thread].next(a)) {
        if (a->conn->is_read_open()) a->conn->shutdown_read();
    }
    shutdown_locks[thread].co_lock(rwi_write);
}
