#ifndef __VAR_BUF_HPP__
#define __VAR_BUF_HPP__

#include "config/alloc.hpp"
#include "config/args.hpp"
#include "conn_fsm.hpp"
#include <stdarg.h>
#include "utils.hpp"

#define MAX_MESSAGE_SIZE 500

// TODO: sizeof(linked_buf_t) should be divisable by 512 so for allocation purposes }

/*! \class linked_buf_t
 *  \brief linked version of iobuf_t
 *  \param _size the maximum bytes that can be stored in one link 
 */
struct linked_buf_t : public buffer_base_t<IO_BUFFER_SIZE>,
                      public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, linked_buf_t> {
    private:
        linked_buf_t    *next;
        int           nbuf; // !< the total number of bytes in the buffer
        int           nsent; // !< how much of the buffer has been sent so far
    public:
        bool          gc_me; // !< whether the buffer could benefit from garbage collection
    public:
        typedef enum {
            linked_buf_outstanding = 0,
            linked_buf_empty,
            linked_buf_num_states,
        } linked_buf_state_t;


    public:
        linked_buf_t() : next(NULL), nbuf(0), nsent(0), gc_me(false) {}
        ~linked_buf_t() {
            if (next != NULL)
                delete next;
        }

        /*! \brief grow the chain of linked_bufs by one
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
            if (nbuf + ninput <= IO_BUFFER_SIZE) {
                memcpy(this->buf + nbuf, input, ninput);
                nbuf += ninput;
            } else if (nbuf < IO_BUFFER_SIZE) {
                //we need to split the input across 2 links
                int free_space = IO_BUFFER_SIZE - nbuf;
                append(input, free_space);
                grow();
                next->append(input + free_space, ninput - free_space);
            } else {
                if (!next) grow(); // ?
                next->append(input, ninput);
            }
        }
        
        void printf(const char *format_str, ...) {
            char buffer[MAX_MESSAGE_SIZE];
            va_list args;
            va_start(args, format_str);
            int count = vsnprintf(buffer, MAX_MESSAGE_SIZE, format_str, args);
            check("Message too big (increase MAX_MESSAGE_SIZE)", count == MAX_MESSAGE_SIZE);
            va_end(args);
            append(buffer, count);
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
        linked_buf_state_t send(net_conn_t *conn) {
            linked_buf_state_t res;
            if (nsent < nbuf) {
                int sz = conn->write_nonblocking(this->buf + nsent, nbuf - nsent);
                if(sz < 0) {
                    if(errno == EAGAIN || errno == EWOULDBLOCK)
                        res = linked_buf_outstanding;
                    else
                        fail("Error sending to socket");
                } else {
                    nsent += sz;
                    if (next == NULL) {
                        //if this is the last link in the chain we slide the buffer
                        memmove(this->buf, this->buf + nsent, nbuf - nsent);
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
                            res = next->send(conn);
                        }
                    } else {
                        res = linked_buf_outstanding;
                    }
                }
            } else if (next != NULL) {
                res = next->send(conn);
            } else {
                res = linked_buf_empty;
            }
            return res;
        }

        /*! \brief delete buffers that have already been fully sent out
         *  \return a pointer to the new head
         */
        linked_buf_t *garbage_collect() {
            if (nbuf == IO_BUFFER_SIZE && nbuf == nsent) {
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

#endif // __VAR_BUF_HPP__
