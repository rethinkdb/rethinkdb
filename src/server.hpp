
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "logger.hpp"
#include "btree/key_value_store.hpp"
#include "conn_acceptor.hpp"
#include "utils.hpp"

/* There is one server_t per server (obviously). It acts as a "master FSM" that is
responsible for the entire lifetime of the server. It creates and destroys the
loggers, caches, and connection acceptor. It does NOT create the thread pool -- instead,
main() creates the thread pool and then creates the server within the thread pool. */

struct server_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, server_t>,
    public home_cpu_mixin_t,
    public log_controller_t::ready_callback_t,
    public store_t::ready_callback_t,
    public conn_acceptor_t::shutdown_callback_t,
    public store_t::shutdown_callback_t,
    public log_controller_t::shutdown_callback_t
{
    
    server_t(cmd_config_t *config, thread_pool_t *tp);
    void do_start();
    void shutdown();   // Can be called from any thread

    cmd_config_t *cmd_config;
    thread_pool_t *thread_pool;
    
    log_controller_t log_controller;
    store_t store;
    conn_acceptor_t conn_acceptor;
    
    int messages_out;
    
private:
    void do_start_loggers();
    void on_logger_ready();
    void do_start_store();
    void on_store_ready();
    void do_start_conn_acceptor();
    
    struct interrupt_message_t :
        public cpu_message_t
    {
        server_t *server;
        void on_cpu_switch() {
            server->do_shutdown();
        }
    } interrupt_message;
    
    void do_shutdown();
    void do_shutdown_conn_acceptor();
    void on_conn_acceptor_shutdown();
    void do_shutdown_store();
    void on_store_shutdown();
    void do_shutdown_loggers();
    void on_logger_shutdown();
    void do_stop_threads();
};

#endif // __SERVER_HPP__

