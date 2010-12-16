
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "logger.hpp"
#include "btree/key_value_store.hpp"
#include "conn_acceptor.hpp"
#include "utils.hpp"
#include "perfmon.hpp"

/* There is one server_t per server (obviously). It acts as a "master FSM" that is
responsible for the entire lifetime of the server. It creates and destroys the
loggers, caches, and connection acceptor. It does NOT create the thread pool -- instead,
main() creates the thread pool and then creates the server within the thread pool. */

class flush_message_t;

class server_t :
    public home_thread_mixin_t,
    public logger_ready_callback_t,
    public btree_key_value_store_t::check_callback_t,
    public btree_key_value_store_t::ready_callback_t,
    public conn_acceptor_t::shutdown_callback_t,
    public btree_key_value_store_t::shutdown_callback_t,
    public logger_shutdown_callback_t
{
    friend class flush_message_t;

public:
    server_t(cmd_config_t *config, thread_pool_t *tp);
    void do_start();
    void shutdown();   // Can be called from any thread

    struct all_gc_disabled_callback_t {
        bool multiple_users_seen;

        all_gc_disabled_callback_t() : multiple_users_seen(false) { }
        virtual void on_gc_disabled() = 0;
        virtual ~all_gc_disabled_callback_t() {}
    };
    bool disable_gc(all_gc_disabled_callback_t *);

    struct all_gc_enabled_callback_t {
        bool multiple_users_seen;
        
        all_gc_enabled_callback_t() : multiple_users_seen(false) { }
        virtual void on_gc_enabled() = 0;
        virtual ~all_gc_enabled_callback_t() {}
    };
    bool enable_gc(all_gc_enabled_callback_t *);

    cmd_config_t *cmd_config;
    thread_pool_t *thread_pool;

    btree_key_value_store_t *store;
    conn_acceptor_t conn_acceptor;

#ifdef REPLICATION_ENABLED
    conn_acceptor_t *replication_acceptor;
#endif
    
private:

    bool do_disable_gc(all_gc_disabled_callback_t *cb);
    bool do_enable_gc(all_gc_enabled_callback_t *cb);
    static conn_handler_t *create_request_handler(net_conn_t *conn, void *server);
    
    int messages_out;
    
    void do_start_loggers();
    void on_logger_ready();
    void do_check_store();
    void on_store_check(bool is_existing);
    void do_start_store();
    void on_store_ready();
    void do_start_conn_acceptor();
    
    struct interrupt_message_t :
        public thread_message_t
    {
        server_t *server;
        void on_thread_switch() {
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
    void do_message_flush();
    void on_message_flush();
    void do_stop_threads();

    class gc_toggler_t : public standard_serializer_t::gc_disable_callback_t {
    public:
        explicit gc_toggler_t(server_t *server);
        bool disable_gc(all_gc_disabled_callback_t *cb);
        bool enable_gc(all_gc_enabled_callback_t *cb);
        
        void on_gc_disabled();

    private:

        enum toggle_state_t {
            enabled,
            disabling,
            disabled
        };

        toggle_state_t state_;
        int num_disabled_serializers_;
        typedef std::vector<all_gc_disabled_callback_t *> callback_vector_t;
        callback_vector_t callbacks_;

        server_t *server_;

        DISABLE_COPYING(gc_toggler_t);
    };

    gc_toggler_t toggler;
};

#endif // __SERVER_HPP__

