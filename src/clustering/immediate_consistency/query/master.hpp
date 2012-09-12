#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "clustering/generic/multi_throttling_server.hpp"
#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/query/master_metadata.hpp"

/* Each shard has a `master_t` on its primary machine. The `master_t` is
responsible for receiving queries from the machines that the clients connect to
and forwarding those queries to the `broadcaster_t`. Specifically, the class
`master_access_t`, which is instantiated by `cluster_namespace_interface_t`,
sends the queries to the `master_t`.

`master_t` internally contains a `multi_throttling_server_t`, which is
responsible for throttling queries from the different `master_access_t`s. */

template<class protocol_t>
class master_t {
public:
    class ack_checker_t : public home_thread_mixin_t {
    public:
        virtual bool is_acceptable_ack_set(const std::set<peer_id_t> &acks) = 0;

        ack_checker_t() { }
    protected:
        virtual ~ack_checker_t() { }

    private:
        DISABLE_COPYING(ack_checker_t);
    };

    master_t(mailbox_manager_t *mm, ack_checker_t *ac,
             typename protocol_t::region_t r,
             broadcaster_t<protocol_t> *b) THROWS_ONLY(interrupted_exc_t);

    ~master_t();

    master_business_card_t<protocol_t> get_business_card();

private:
    class client_t {
    public:
        client_t(
                master_t *p,
                UNUSED const typename master_business_card_t<protocol_t>::inner_client_business_card_t &) :
            parent(p) { }
        void perform_request(
                const typename master_business_card_t<protocol_t>::request_t &,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
    private:
        master_t *parent;
        fifo_enforcer_sink_t fifo_sink;
    };

    mailbox_manager_t *mailbox_manager;
    ack_checker_t *ack_checker;
    broadcaster_t<protocol_t> *broadcaster;
    typename protocol_t::region_t region;

    /* See note in `client_t::perform_request()` for what this is about */
    cond_t shutdown_cond;

    multi_throttling_server_t<
            typename master_business_card_t<protocol_t>::request_t,
            typename master_business_card_t<protocol_t>::inner_client_business_card_t,
            master_t *,
            client_t
            > multi_throttling_server;

    DISABLE_COPYING(master_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_ */
