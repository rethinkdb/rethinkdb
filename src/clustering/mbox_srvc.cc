#include "clustering/mbox_srvc.hpp"
#include "clustering/cluster.hpp"

void mailbox_srvc_t::handle(cluster_peer_t *sndr) {
    if (cluster_mailbox_t *mbox = get_cluster().get_mailbox(msg.id())) {
        cluster_inpipe_t inpipe(sndr->conn.get(), msg.length());
        coro_t::spawn_now(boost::bind(&cluster_mailbox_t::unserialize, mbox, &inpipe));
        inpipe.to_signal_when_done.wait();
    } else {
        if (msg.has_type()) { 
            logERR("Mailbox msg of type: %s from peer: %d, sent to nonexistant mailbox:%d\n", msg.type().c_str(), sndr->id, msg.id());
        } else { 
            logERR("Mailbox msg of unspecified type from peer: %d, sent to nonexistant mailbox:%d\n", sndr->id, msg.id());
        }
        /* consume the msg so that we can keep going */
        sndr->conn->pop(msg.length());
    }
}
