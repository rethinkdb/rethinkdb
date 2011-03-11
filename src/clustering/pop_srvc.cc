#include "clustering/pop_srvc.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include "clustering/cluster.hpp"

void join_propose_srvc_t::handle(cluster_peer_t *sndr) {
    population::Join_respond respond;

    int prop_id = msg.addr().id();
    if (get_cluster().peers.find(prop_id) == get_cluster().peers.end()) {
        /* no conflict send out the ack */
        get_cluster().peers[prop_id] = boost::make_shared<cluster_peer_t>(msg.addr().ip(), msg.addr().port(), sndr->id, prop_id); /* NULL because we don't have a connection to this peer yet */
        get_cluster().peers[prop_id]->set_state(cluster_peer_t::join_confirmed);

        *(respond.mutable_addr()) = msg.addr();
        respond.set_agree(true);
    } else {
        /* if this assert gets tripped it means that one of our peers
         * has attempted to add a peer to the network with an id that
         * it should already know about */
        guarantee(get_cluster().peers[prop_id]->get_state() == cluster_peer_t::join_proposed || get_cluster().peers[prop_id]->get_state() == cluster_peer_t::join_confirmed);

        /* we have a proposal conflict, that needs to be resolved */
        if (get_cluster().peers[prop_id]->proposer < sndr->id) {
            /* someone with lower id already requested this so we send a rejection */

            *(respond.mutable_addr()) = msg.addr();
            respond.set_agree(false);
        } else {
            /* this request supersedes the other */
            get_cluster().peers[prop_id].reset(new cluster_peer_t(msg.addr().ip(), msg.addr().port(), sndr->id, prop_id));

            /* notice, we have just reneged on our previous
             * confirmation this only works because the fact that we
             * received this conflicting proposal means that the second
             * proposer will reject the first proposer */
            *(respond.mutable_addr()) = msg.addr();
            respond.set_agree(true);
        }
    }

    sndr->write(&respond);
}

void join_mk_official_srvc_t::handle(cluster_peer_t *sndr) {
    population::Join_ack_official ack_official;

    int prop_id = msg.addr().id();

    //TODO all these guarantees should really be exception instead, since they
    //can happen during runtime if someone starts throwing garbage over the
    //network
    guarantee(get_cluster().peers.find(prop_id) != get_cluster().peers.end());
    guarantee(get_cluster().peers[prop_id]->get_state() == cluster_peer_t::join_confirmed);
    guarantee(get_cluster().peers[prop_id]->proposer == sndr->id);

    get_cluster().peers[prop_id]->set_state(cluster_peer_t::join_official);

    *(ack_official.mutable_addr()) = msg.addr();
    sndr->write(&ack_official);
}

void kill_propose_srvc_t::handle(cluster_peer_t *sndr) {
    /* right now, this is a particularly homicidal cluster, anytime someone
     * suggests we kill a node, we just go along with it in the future we
     * should perhaps check to see that we actually want to kill this node */
    population::Kill_respond respond;

    int prop_id = msg.addr().id();
    guarantee(get_cluster().peers.find(prop_id) != get_cluster().peers.end());
    get_cluster().peers[prop_id]->set_state(cluster_peer_t::kill_confirmed);

    *(respond.mutable_addr()) = msg.addr();
    respond.set_agree(true);

    sndr->write(&respond);
}

void kill_mk_official_srvc_t::handle(cluster_peer_t *sndr) {
    int prop_id = msg.addr().id();

    //TODO all these guarantees should really be exception instead, since they
    //can happen during runtime if someone starts throwing garbage over the
    //network
    guarantee(get_cluster().peers.find(prop_id) != get_cluster().peers.end());
    guarantee(get_cluster().peers[prop_id]->get_state() == cluster_peer_t::kill_confirmed || get_cluster().peers[prop_id]->get_state() == cluster_peer_t::killed);

    if (get_cluster().peers[prop_id]->get_state() == cluster_peer_t::kill_confirmed) {
        get_cluster().peers[prop_id]->stop_servicing();
        get_cluster().peers[prop_id]->set_state(cluster_peer_t::killed);
    }

    get_cluster().print_peers();
}
