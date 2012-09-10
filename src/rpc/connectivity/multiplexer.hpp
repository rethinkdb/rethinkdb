#ifndef RPC_CONNECTIVITY_MULTIPLEXER_HPP_
#define RPC_CONNECTIVITY_MULTIPLEXER_HPP_

#include "rpc/connectivity/messages.hpp"

/* `message_multiplexer_t` is used when you want multiple components to share a
`message_service_t`. Here's an example of how one might use it:

    connectivity_cluster_t cluster;
    message_multiplexer_t multiplexer(&cluster);

    message_multiplexer_t::client_t app_x_client(&multiplexer, 'X');
    app_x_t app_x(&app_x_client);
    message_multiplexer_t::client_t::run_t app_x_run(&app_x_client, &app_x);

    message_multiplexer_t::client_t app_y_client(&multiplexer, 'Y');
    app_y_t app_y(&app_y_client);
    message_multiplexer_t::client_t::run_t app_y_run(&app_y_client, &app_y);

    message_multiplexer_t::run_t multiplexer_run(&multiplexer);
    connectivity_cluster_t::run_t cluster_run(&multiplexer_run);

    ... do stuff ...

    // destructors take care of shutting everything down

*/

class message_multiplexer_t {
public:
    typedef unsigned char tag_t;
    static const int max_tag = 256;
    class run_t : public message_handler_t {
    public:
        explicit run_t(message_multiplexer_t *);
        ~run_t();
    private:
        void on_message(peer_id_t, read_stream_t *);
        message_multiplexer_t *const parent;
    };
    class client_t : public message_service_t {
    public:
        class run_t {
        public:
            run_t(client_t *, message_handler_t *);
            ~run_t();
        private:
            friend class message_multiplexer_t::run_t;
            client_t *const parent;
            message_handler_t *const message_handler;
        };
        client_t(message_multiplexer_t *, tag_t tag);
        ~client_t();
        connectivity_service_t *get_connectivity_service();
        void send_message(peer_id_t, send_message_write_callback_t *callback);
    private:
        friend class message_multiplexer_t;
        message_multiplexer_t *const parent;
        const tag_t tag;
        run_t *run;
    };
    explicit message_multiplexer_t(message_service_t *super_ms);
    ~message_multiplexer_t();
private:
    friend class run_t;
    friend class client_t;
    friend class client_t::run_t;
    message_service_t *const message_service;
    client_t *clients[max_tag];
    run_t *run;
};

#endif /* RPC_CONNECTIVITY_MULTIPLEXER_HPP_ */
