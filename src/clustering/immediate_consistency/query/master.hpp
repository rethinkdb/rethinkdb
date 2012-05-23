#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_

#include <map>

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/query/metadata.hpp"
#include "containers/uuid.hpp"

/* The point at which we begin throttling. */
#define THROTTLING_THRESHOLD 1000
/* The minimum number of operations we will ever offer someone who asks for an
 * allocation. */
#define MINIMUM_ALLOCATION 5
/* The number of operations we allocate when we're not doing any throttling. */
#define BASE_ALLOCATION 100

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

    master_t(
            mailbox_manager_t *mm,
            ack_checker_t *ac,
            watchable_variable_t<std::map<master_id_t, master_business_card_t<protocol_t> > > *md,
            mutex_assertion_t *mdl,
            typename protocol_t::region_t region,
            broadcaster_t<protocol_t> *b)
            THROWS_ONLY(interrupted_exc_t) :
        mailbox_manager(mm),
        ack_checker(ac),
        broadcaster(b),
        registrar(mm, this),
        read_mailbox(mailbox_manager, boost::bind(&master_t<protocol_t>::on_read,
                                                  this, _1, _2, _3, _4, _5, auto_drainer_t::lock_t(&drainer))),
        read_allocation_mailbox(mailbox_manager, boost::bind(&master_t<protocol_t>::allocate_reads, this, _1), mailbox_callback_mode_inline),
        write_mailbox(mailbox_manager, boost::bind(&master_t<protocol_t>::on_write,
                                                   this, _1, _2, _3, _4, _5, auto_drainer_t::lock_t(&drainer))),
        write_allocation_mailbox(mailbox_manager, boost::bind(&master_t<protocol_t>::allocate_writes, this, _1), mailbox_callback_mode_inline),
        master_directory(md), master_directory_lock(mdl),
        uuid(generate_uuid()) {

        rassert(ack_checker);

        master_business_card_t<protocol_t> bcard(
            region,
            read_allocation_mailbox.get_address(), read_mailbox.get_address(), 
            write_allocation_mailbox.get_address(), write_mailbox.get_address(),
            registrar.get_business_card());
        mutex_assertion_t::acq_t master_directory_lock_acq(master_directory_lock);
        std::map<master_id_t, master_business_card_t<protocol_t> > master_map = master_directory->get_watchable()->get();
        master_map.insert(std::make_pair(uuid, bcard));
        master_directory->set_value(master_map);
    }

    ~master_t() {
        mutex_assertion_t::acq_t master_directory_lock_acq(master_directory_lock);
        std::map<master_id_t, master_business_card_t<protocol_t> > master_map = master_directory->get_watchable()->get();
        master_map.erase(uuid);
        master_directory->set_value(master_map);
    }

private:
    struct parser_lifetime_t {
        parser_lifetime_t(master_t *m, namespace_interface_business_card_t bc) : m_(m), namespace_interface_id_(bc.namespace_interface_id) {
            m->sink_map.insert(std::pair<namespace_interface_id_t, parser_lifetime_t *>(bc.namespace_interface_id, this));
            send(m->mailbox_manager, bc.ack_address);
        }
        ~parser_lifetime_t() {
            m_->sink_map.erase(namespace_interface_id_);
        }

        auto_drainer_t *drainer() { return &drainer_; }
        fifo_enforcer_sink_t *sink() { return &sink_; }
    private:
        master_t *m_;
        namespace_interface_id_t namespace_interface_id_;
        fifo_enforcer_sink_t sink_;
        auto_drainer_t drainer_;

        DISABLE_COPYING(parser_lifetime_t);
    };

    void on_read(namespace_interface_id_t parser_id, typename protocol_t::read_t read, order_token_t otok, fifo_enforcer_read_token_t token,
            mailbox_addr_t<void(boost::variant<typename protocol_t::read_response_t, std::string>)> response_address,
            auto_drainer_t::lock_t keepalive)
            THROWS_NOTHING
    {
        try {
            keepalive.assert_is_holding(&drainer);
            boost::variant<typename protocol_t::read_response_t, std::string> reply;
            try {
                typename std::map<namespace_interface_id_t, parser_lifetime_t *>::iterator it = sink_map.find(parser_id);
                if (it == sink_map.end()) {
                    /* We got a message from a parser that already deregistered
                    itself. This happened because the deregistration message
                    raced ahead of the query. Ignore it. */
                    return;
                }

                auto_drainer_t::lock_t auto_drainer_lock(it->second->drainer());
                fifo_enforcer_sink_t::exit_read_t exiter(it->second->sink(), token);
                reply = broadcaster->read(read, &exiter, otok, auto_drainer_lock.get_drain_signal());
            } catch (cannot_perform_query_exc_t e) {
                reply = e.what();
            }
            send(mailbox_manager, response_address, reply);
        } catch (interrupted_exc_t) {
            /* Bail out and don't even send a response. The reason an exception
            was thrown is because the `master_t` is being destroyed, and the
            client will find out some other way. */
        }
    }

    void allocate_reads(mailbox_addr_t<void(int)> response_address) {
        send(mailbox_manager, response_address, 100);
    }

    void on_write(namespace_interface_id_t parser_id, typename protocol_t::write_t write, order_token_t otok, fifo_enforcer_write_token_t token,
                  mailbox_addr_t<void(boost::variant<typename protocol_t::write_response_t, std::string>)> response_address,
                  auto_drainer_t::lock_t keepalive)
        THROWS_NOTHING
    {
        rassert(allocated_but_unsent_writes > 0, "Someone must have sent a write without first asking for an allocation.");
        allocated_but_unsent_writes--;
        try {
            keepalive.assert_is_holding(&drainer);
            boost::variant<typename protocol_t::write_response_t, std::string> reply;
            try {
                typename std::map<namespace_interface_id_t, parser_lifetime_t *>::iterator it = sink_map.find(parser_id);
                if (it == sink_map.end()) {
                    /* We got a message from a parser that already deregistered
                    itself. This happened because the deregistration message
                    raced ahead of the query. Ignore it. */
                    return;
                }

                class ac_t : public broadcaster_t<protocol_t>::ack_callback_t {
                public:
                    explicit ac_t(master_t *p) : parent(p) { }
                    bool on_ack(peer_id_t peer) {
                        ack_set.insert(peer);
                        return parent->ack_checker->is_acceptable_ack_set(ack_set);
                    }
                    master_t *parent;
                    std::set<peer_id_t> ack_set;
                } ack_checker(this);

                auto_drainer_t::lock_t auto_drainer_lock(it->second->drainer());
                fifo_enforcer_sink_t::exit_write_t exiter(it->second->sink(), token);
                reply = broadcaster->write(write, &exiter, &ack_checker, otok, auto_drainer_lock.get_drain_signal());
            } catch (cannot_perform_query_exc_t e) {
                reply = e.what();
            }
            send(mailbox_manager, response_address, reply);
        } catch (interrupted_exc_t) {
            /* See note in `on_read()` */
        }
    }

    void allocate_writes(mailbox_addr_t<void(int)> response_address) {
        int longest_queue_size = broadcaster->longest_queue_size();
        int writes_to_allocate;
        if (longest_queue_size < THROTTLING_THRESHOLD) {
            writes_to_allocate = BASE_ALLOCATION;
        } else {
            writes_to_allocate = int(float(BASE_ALLOCATION) / float(longest_queue_size - THROTTLING_THRESHOLD)) + MINIMUM_ALLOCATION;
        }
        allocated_but_unsent_writes += writes_to_allocate;
        send(mailbox_manager, response_address, writes_to_allocate);
    }

    mailbox_manager_t *mailbox_manager;
    ack_checker_t *ack_checker;
    broadcaster_t<protocol_t> *broadcaster;
    std::map<namespace_interface_id_t, parser_lifetime_t *> sink_map;
    auto_drainer_t drainer;

    registrar_t<namespace_interface_business_card_t, master_t *, parser_lifetime_t> registrar;

    typename master_business_card_t<protocol_t>::read_mailbox_t read_mailbox;
    typename master_business_card_t<protocol_t>::allocation_mailbox_t read_allocation_mailbox;

    typename master_business_card_t<protocol_t>::write_mailbox_t write_mailbox;
    typename master_business_card_t<protocol_t>::allocation_mailbox_t write_allocation_mailbox;

    watchable_variable_t<std::map<master_id_t, master_business_card_t<protocol_t> > > *master_directory;
    mutex_assertion_t *master_directory_lock;
    master_id_t uuid;

    int allocated_but_unsent_writes;

    DISABLE_COPYING(master_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_ */
