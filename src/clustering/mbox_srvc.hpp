#ifndef _CLUSTERING_MBOX_SRVC_HPP_
#define _CLUSTERING_MBOX_SRVC_HPP_

#include "clustering/mailbox.pb.h"
#include "clustering/srvc.hpp"

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

#endif
