
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "logger.hpp"
#include "btree/key_value_store.hpp"
#include "conn_acceptor.hpp"
#include "utils.hpp"
#include "perfmon.hpp"
#include "server/cmd_args.hpp"

/* run_server() is called by main() once it determines that the subprogram to be run
is 'serve' or 'create'. */

int run_server(int argc, char *argv[]);

/* server_main() is meant to be run in a coroutine. It should be started at server
start. It will return right when the server shuts down. */

void server_main(cmd_config_t *cmd_config, thread_pool_t *thread_pool);

/* server_t is mostly a holdover from before we had coroutines. It should eventually
disappear. Right now it only exists so pointers to it can be passed around. */

class server_t :
    public home_thread_mixin_t
{
public:
    server_t(cmd_config_t *cmd_config, thread_pool_t *thread_pool)
        : cmd_config(cmd_config), thread_pool(thread_pool), toggler(this)
    {
    }

    void shutdown();   // Can be called from any thread

    struct all_gc_disabled_callback_t {
        bool multiple_users_seen;

        all_gc_disabled_callback_t() : multiple_users_seen(false) { }
        virtual void on_gc_disabled() = 0;
        virtual ~all_gc_disabled_callback_t() {}
    };
    bool disable_gc(all_gc_disabled_callback_t *);
    bool do_disable_gc(all_gc_disabled_callback_t *cb);

    struct all_gc_enabled_callback_t {
        bool multiple_users_seen;
        
        all_gc_enabled_callback_t() : multiple_users_seen(false) { }
        virtual void on_gc_enabled() = 0;
        virtual ~all_gc_enabled_callback_t() {}
    };
    bool enable_gc(all_gc_enabled_callback_t *);
    bool do_enable_gc(all_gc_enabled_callback_t *cb);

    cmd_config_t *cmd_config;
    btree_key_value_store_t *store;
    thread_pool_t *thread_pool;

private:
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

    } toggler;
};

#endif // __SERVER_HPP__

