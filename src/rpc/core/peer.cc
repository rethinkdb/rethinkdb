#include "rpc/core/peer.hpp"
#include "rpc/core/protob.hpp"
#include "arch/linux/coroutines.hpp"

void cluster_peer_t::add_srvc(msg_srvc_ptr srvc) {
    srvcs.push_back(srvc);
    srvc->on_push(this);
}

void cluster_peer_t::remove_srvc(msg_srvc_ptr srvc) {
    srvcs.remove(srvc);
}

/* void cluster_peer_t::start_servicing() {
    coro_t::spawn(boost::bind(&cluster_peer_t::_start_servicing, this));
} */

void cluster_peer_t::start_servicing() {
    logINF("Services up for peer: %d\n", id);
    keep_servicing = true;

    while (keep_servicing) {
        bool handled_a_message = false;
        for (srvc_list_t::iterator it = srvcs.begin(); it != srvcs.end(); it++) {
            if (peek((*it)->_msg) && (*it)->want_message(this)) {
                pop_protob(conn.get());
                (*it)->on_receipt(this);
                if ((*it)->one_off) it = srvcs.erase(it);
                handled_a_message = true;
                break;
            } else {
            }
        }
        if (!handled_a_message) {
            /* a message came in on the wire that we didn't know how to handle */
            header::hdr hdr;
            pop_protob(conn.get(), &hdr);
            logERR("Unhandled message of type: %s\n", hdr.type().c_str());
        }
    }
}

void cluster_peer_t::stop_servicing() {
    conn.reset();
}
