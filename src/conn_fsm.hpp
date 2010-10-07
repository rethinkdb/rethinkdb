#ifndef __CONN_FSM_HPP__
#define __CONN_FSM_HPP__

#include "containers/intrusive_list.hpp"
#include "containers/var_buf.hpp"
#include "arch/arch.hpp"
#include "request_handler/request_handler.hpp"
#include "event.hpp"
#include "corefwd.hpp"
#include <stdarg.h>

// TODO: the lifetime of conn_fsm isn't well defined - some objects
// may persist for far longer than others. The small object dynamic
// pool allocator is designed for objects that have roughly the same
// lifetime. We should use a different allocator for objects like
// conn_fsm (and btree buffers).

struct data_transferred_callback {
    virtual void on_data_transferred() = 0;
    virtual ~data_transferred_callback() {}
};

struct conn_fsm_shutdown_callback_t {
    virtual void on_conn_fsm_quit() = 0;
    virtual void on_conn_fsm_shutdown() = 0;
};

// The actual state structure
struct conn_fsm_t :
    public intrusive_list_node_t<conn_fsm_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, conn_fsm_t>,
    public net_conn_callback_t
{
public:
    typedef buffer_t<IO_BUFFER_SIZE> iobuf_t;
    
public:
    // Possible transition results
    enum result_t {
        fsm_invalid,
        fsm_no_data_in_socket,
        fsm_quit_connection,
        fsm_transition_ok,
    };

    enum state_t {
        //! Socket is connected, is in a clean state (no outstanding ops) and ready to go
        fsm_socket_connected,

        //! Socket has received an incomplete packet and waiting for the rest of the command
        fsm_socket_recv_incomplete,

        //! We sent a msg over the socket but were only able to send a partial packet
        fsm_socket_send_incomplete,
        
        //! Waiting for IO initiated by the BTree to complete
        fsm_btree_incomplete,

        //! There is outstanding data in rbuff
        fsm_outstanding_data,
    };
    
public:
    conn_fsm_t(net_conn_t *conn, conn_fsm_shutdown_callback_t *cb, request_handler_t *rh);
    ~conn_fsm_t();

public:
    /* Called by conn_acceptor_t when the server is being shut down. */
    void start_quit();
private:
    bool quitting; /* !< if true don't issue anymore requests just let them finish and then quit */

public:
    void consume(unsigned int bytes);
    
    /* Called by net_conn_t */
    void on_net_conn_readable();
    void on_net_conn_writable();
    void on_net_conn_close();
    
public:
    net_conn_t *conn;
    state_t state;
    bool corked; //whether or not we should buffer our responses

    // A buffer with IO communication (possibly incomplete). The nrbuf
    // variable indicates how many bytes are stored in the buffer.
    char *rbuf;
    linked_buf_t *sbuf;
    unsigned int nrbuf;
    char *ext_rbuf;
    unsigned int ext_nrbuf;
    unsigned int ext_size;
    //state_t previous_state;
    data_transferred_callback *cb;
    /*! \warning {If req_handler::parse_request returns op_req_parallelizable then it MUST NOT send an et_request_complete,
     *              if it returns op_req_complex then it MUST send an et_request_complete event}
     */
    request_handler_t *req_handler;

    conn_fsm_shutdown_callback_t *shutdown_callback;

    void fill_external_buf(byte *external_buf, unsigned int size, data_transferred_callback *callback);
    void send_external_buf(byte *external_buf, unsigned int size, data_transferred_callback *callback);
    void dummy_sock_event();

    void do_transition_and_handle_result(event_t *event);

private:
    void check_external_buf();
    
    result_t do_transition(event_t *event);
    
    result_t fill_buf(void *buf, unsigned int *bytes_filled, unsigned int total_length);

    result_t fill_rbuf();
    result_t fill_ext_rbuf();

    result_t read_data(event_t *event);

    result_t do_socket_send_incomplete(event_t *event);
    result_t do_fsm_btree_incomplete(event_t *event);
    result_t do_fsm_outstanding_req(event_t *event);

    
    void send_msg_to_client();
    void init_state();
    void return_to_socket_connected();
};

#endif // __CONN_FSM_HPP__

