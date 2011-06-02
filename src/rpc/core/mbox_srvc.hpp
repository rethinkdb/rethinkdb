#ifndef _RPC_CORE_MBOX_SRVC_HPP_
#define _RPC_CORE_MBOX_SRVC_HPP_

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

struct cluster_peer_inpipe_t : public cluster_inpipe_t {
    void do_read(void *buf, size_t size) {
        try {
            conn->read(buf, size);
        } catch (tcp_conn_t::read_closed_exc_t) {}
    }
    cluster_peer_inpipe_t(tcp_conn_t *conn, int bytes) : cluster_inpipe_t(bytes), conn(conn) { }
    tcp_conn_t *conn;
};

#endif /* _RPC_CORE_MBOX_SRVC_HPP_ */
