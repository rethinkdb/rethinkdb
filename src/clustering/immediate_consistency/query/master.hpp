#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/query/metadata.hpp"

#define ALLOCATION_CHUNK 50
#define TARGET_OPERATION_QUEUE_LENGTH 5000
#define MAX_ALLOCATION 1000

template<class protocol_t>
class master_t {
public:
    class ack_checker_t {
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

    master_business_card_t<protocol_t> get_business_card();

private:
    struct parser_lifetime_t {
        parser_lifetime_t(master_t *m, master_access_business_card_t bc)
            : allocate_mailbox(bc.allocation_address), allocated_ops(0),
              m_(m), master_access_id_(bc.master_access_id)
        {
            m->sink_map.insert(std::pair<master_access_id_t, parser_lifetime_t *>(bc.master_access_id, this));
            send(m->mailbox_manager, bc.ack_address);
        }
        ~parser_lifetime_t() {
            m_->sink_map.erase(master_access_id_);
       }

        auto_drainer_t *drainer() { return &drainer_; }
        fifo_enforcer_sink_t *sink() { return &sink_; }
        mailbox_addr_t<void(int)> allocate_mailbox;
        int allocated_ops;
    private:
        master_t *m_;
        master_access_id_t master_access_id_;
        fifo_enforcer_sink_t sink_;
        auto_drainer_t drainer_;

        DISABLE_COPYING(parser_lifetime_t);
    };

    void on_read(master_access_id_t parser_id, typename protocol_t::read_t read, order_token_t otok, fifo_enforcer_read_token_t token,
                 mailbox_addr_t<void(boost::variant<typename protocol_t::read_response_t, std::string>)> response_address,
                 auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    void on_write(master_access_id_t parser_id, typename protocol_t::write_t write, order_token_t otok, fifo_enforcer_write_token_t token,
                  mailbox_addr_t<void(boost::variant<typename protocol_t::write_response_t, std::string>)> response_address,
                  auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    void consider_allocating_more_writes();

    void allocate_more_writes(mailbox_addr_t<void(int)> addr, UNUSED auto_drainer_t::lock_t keepalive);

    mailbox_manager_t *mailbox_manager;
    ack_checker_t *ack_checker;
    broadcaster_t<protocol_t> *broadcaster;
    typename protocol_t::region_t region;

    std::map<master_access_id_t, parser_lifetime_t *> sink_map;
    int enqueued_writes;

    auto_drainer_t drainer;

    registrar_t<master_access_business_card_t, master_t *, parser_lifetime_t> registrar;

    typename master_business_card_t<protocol_t>::read_mailbox_t read_mailbox;

    typename master_business_card_t<protocol_t>::write_mailbox_t write_mailbox;

    DISABLE_COPYING(master_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_ */
