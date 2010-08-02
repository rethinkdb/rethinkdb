
#ifndef __CONN_FSM_HPP__
#define __CONN_FSM_HPP__

#include "containers/intrusive_list.hpp"
#include "arch/resource.hpp"
#include "request_handler/request_handler.hpp"
#include "event.hpp"
#include "btree/fsm.hpp"
#include "corefwd.hpp"

// TODO: the lifetime of conn_fsm isn't well defined - some objects
// may persist for far longer than others. The small object dynamic
// pool allocator (currently defined as alloc_t in config_t) is
// designed for objects that have roughly the same lifetime. We should
// use a different allocator for objects like conn_fsm (and btree
// buffers).

// The actual state structure
template<class config_t>
struct conn_fsm : public intrusive_list_node_t<conn_fsm<config_t> >,
                  public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, conn_fsm<config_t> >
{
public:
    typedef typename config_t::iobuf_t iobuf_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::request_t request_t;
    typedef typename config_t::req_handler_t req_handler_t;
    
public:
    // Possible transition results
    enum result_t {
        fsm_invalid,
        fsm_shutdown_server,
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
    conn_fsm(resource_t _source, event_queue_t *_event_queue);
    ~conn_fsm();
    
    result_t do_transition(event_t *event);
    void consume(unsigned int bytes);

    int get_source() {
        return source;
    }

public:
    /*! \class linked_buf_t
     *  \brief linked version of iobuf_t
     *  \param _size the maximum bytes that can be stored in one link 
     */
    struct linked_buf_t : public iobuf_t {
        private:
            linked_buf_t    *next;
            int             nbuf; // !< the total number of bytes in the buffer
            int             nsent; // !< how much of the buffer has been sent so far
        public:
            bool            gc_me; // !< whether the buffer could benefit from garbage collection
        public:
            typedef enum {
                linked_buf_outstanding = 0,
                linked_buf_empty,
                linked_buf_num_states,
            } linked_buf_state_t;


        public:
            linked_buf_t()
                : iobuf_t(), next(NULL), nbuf(0), nsent(0), gc_me(false) {}
            ~linked_buf_t() {
                if (next != NULL)
                    delete next;
            }

            /*! \brief grow the chain of linked_buf_ts by one
             */
            void grow() {
                if (next == NULL)
                    next = new linked_buf_t();
                else
                    next->grow();
            }

            /*! \brief add data the the end of chain of linked lists
             */
            void append(const char *input, int ninput) {
                if (nbuf + ninput <= iobuf_t::size) {
                    memcpy(iobuf_t::buf + nbuf, input, ninput);
                    nbuf += ninput;
                } else if (nbuf < iobuf_t::size) {
                    //we need to split the input across 2 links
                    int free_space = iobuf_t::size - nbuf;
                    append(input, free_space);
                    grow();
                    next->append(input + free_space, ninput - free_space);
                } else {
                    next->append(input, ninput);
                }
            }

            /*! \brief check if a buffer has outstanding data
             *  \return true if there is outstanding data
             */
            linked_buf_state_t outstanding() {
                if ((nsent < nbuf) || ((next != NULL) && next->outstanding()))
                    return linked_buf_outstanding;
                else
                    return linked_buf_empty;
            }

            /*! \brief try to send as much of the buffer as possible
             *  \return true if there is still outstanding data
             */
            linked_buf_state_t send(int source) {
                linked_buf_state_t res;
                if (nsent < nbuf) {
                    int sz = io_calls_t::write(source, iobuf_t::buf + nsent, nbuf - nsent);
                    if(sz < 0) {
                        if(errno == EAGAIN || errno == EWOULDBLOCK)
                            res = linked_buf_outstanding;
                        else
                            check("Error sending to socket", 1);
                    } else {
                        nsent += sz;
                        if (next == NULL) {
                            //if this is the last link in the chain we slide the buffer
                            memmove(iobuf_t::buf, iobuf_t::buf + nsent, nbuf - nsent);
                            nbuf -= nsent;
                            nsent = 0;
                        }

                        if (nsent == nbuf) {
                            if (next == NULL) {
                                res = linked_buf_empty;
                            } else {
                                //the network handled this buffer without a problem
                                //so maybe we can send more
                                gc_me = true; //ask for garbage collection
                                res = next->send(source);
                            }
                        } else {
                            res = linked_buf_outstanding;
                        }
                    }
                } else if (next != NULL) {
                    res = next->send(source);
                } else {
                    res = linked_buf_empty;
                }
                return res;
            }

            /*! \brief delete buffers that have already been fully sent out
             *  \return a pointer to the new head
             */
            linked_buf_t *garbage_collect() {
                if (nbuf == iobuf_t::size && nbuf == nsent) {
                    if (next == NULL)
                        grow();

                    linked_buf_t *tmp = next;
                    next = NULL;
                    delete this;
                    return tmp->garbage_collect();
                } else {
                    return this;
                }
            }
    };

public:
    int source;
    state_t state;
    bool corked; //whether or not we should buffer our responses

    // A buffer with IO communication (possibly incomplete). The nbuf
    // variable indicates how many bytes are stored in the buffer. The
    // snbuf variable indicates how many bytes out of the buffer have
    // been sent (in case of a send workflow).
    char *rbuf;
    linked_buf_t *sbuf;
    unsigned int nrbuf;
    req_handler_t *req_handler;
    event_queue_t *event_queue;
    request_t *current_request;
    
private:
    result_t fill_rbuf(event_t *event);
    result_t do_socket_send_incomplete(event_t *event);
    result_t do_fsm_btree_incomplete(event_t *event);
    result_t do_fsm_outstanding_req(event_t *event);
    
    void send_msg_to_client();
    void send_err_to_client();
    void init_state();
    void return_to_socket_connected();
};

// Include the implementation
#include "conn_fsm.tcc"

#endif // __CONN_FSM_HPP__

