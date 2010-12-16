#ifndef __CONN_ACCEPTOR_HPP__
#define __CONN_ACCEPTOR_HPP__

#include "arch/arch.hpp"
#include "utils.hpp"
#include "perfmon.hpp"

class conn_handler_t;

/* The conn_acceptor_t is responsible for accepting incoming network connections, creating
objects to deal with them, and shutting down the network connections when the server
shuts down. It uses net_listener_t to actually accept the connections. Each conn_acceptor_t
lasts for the entire lifetime of the server. */

class conn_acceptor_t :
    public net_listener_callback_t,
    public home_thread_mixin_t
{
    friend class conn_handler_t;

public:
    /* When the conn_acceptor_t gets an incoming connection, it calls the provided creator
    function to make an object to handle it. */
    explicit conn_acceptor_t(int port, conn_handler_t *(*creator)(net_conn_t*, void*), void *udata);

    // Returns false on error, true if everything is ok
    bool start();

    struct shutdown_callback_t {
        virtual void on_conn_acceptor_shutdown() = 0;
        virtual ~shutdown_callback_t() {}
    };
    bool shutdown(shutdown_callback_t *cb);

    ~conn_acceptor_t();

private:
    enum state_t {
        state_off,
        state_ready,
        state_shutting_down
    } state;

    int port;

    conn_handler_t *(*creator)(net_conn_t*, void*);
    void *creator_udata;

    net_listener_t *listener;
    int next_thread;
    void on_net_listener_accept(net_conn_t *conn);
    bool create_conn_on_this_core(net_conn_t *conn);

    int n_active_conns;
    intrusive_list_t<conn_handler_t> conn_handlers[MAX_THREADS];

    bool have_shutdown_a_conn();

    bool shutdown_conns_on_this_core();   // Called on each thread
    shutdown_callback_t *shutdown_callback;
};

/* conn_handler_t represents an object that handles a network connection. To use conn_acceptor_t,
subclass conn_handler_t and make the function that you pass to conn_acceptor_t's constructor
return an instance of your own subclass.

When the server is shutting down, it will call the quit() method of every conn_handler_t. The
conn_handler_t should override quit() to start the process of closing the connection. It should
also call on_quit() whenever it starts the process of closing the connection (whether because
quit() was called or because it closed the connection for some other reason). Once it calls
on_quit(), then quit() will not be called. The server will not actually shut down until the
destructor of every conn_handler_t has been called. */

class conn_handler_t :
    public intrusive_list_node_t<conn_handler_t>
{
    friend class conn_acceptor_t;

public:
    conn_handler_t();

    // Should eventually close the socket and destroy the conn_handler_t, but doesn't need to
    // do so immediately.
    virtual void quit() = 0;

    ~conn_handler_t();

private:
    conn_acceptor_t *parent;
    net_conn_t *conn;
};

#endif /* __CONN_ACCEPTOR_HPP__ */
