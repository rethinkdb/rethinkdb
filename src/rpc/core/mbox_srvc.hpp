#ifndef _RPC_CORE_MBOX_SRVC_HPP_
#define _RPC_CORE_MBOX_SRVC_HPP_

#include <istream>
#include "arch/conn_streambuf.hpp"
#include "rpc/core/mailbox.pb.h"
#include "rpc/core/srvc.hpp"
#include "rpc/core/cluster.hpp"

/* mailbox_srvc_t is to be used as an always on srvc, it is responsible for
 * sending mailbox_msg data into the correct mailboxes and sending back errors
 * when something goes wrong */
class mailbox_srvc_t
    : public msg_srvc_t<mailbox::mailbox_msg>
{
public:
    mailbox_srvc_t()
        : msg_srvc_t<mailbox::mailbox_msg>() {}

    bool want_message(cluster_peer_t *) { return true; }

    void handle(cluster_peer_t *);
};

/* Concrete subclass of `cluster_inpipe_t` */

struct cluster_peer_inpipe_t : public checking_inpipe_t {
    void do_read(void *buf, size_t size) {
        try {
            conn->read(buf, size);
        } catch (tcp_conn_t::read_closed_exc_t) {}
    }
    
    rpc_iarchive_t &get_archive() {
        return archive;
    }
    
    cluster_peer_inpipe_t(tcp_conn_t *conn, int bytes) :
            checking_inpipe_t(bytes),
            conn(conn),
            conn_streambuf(conn), 
            conn_stream(&conn_streambuf),
            archive(conn_stream) {
    }
    
    tcp_conn_t *conn;
    tcp_conn_streambuf_t conn_streambuf;
    std::istream conn_stream;
    rpc_iarchive_t archive;
};

#endif /* _RPC_CORE_MBOX_SRVC_HPP_ */
