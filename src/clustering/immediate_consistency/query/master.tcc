#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_TCC__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_TCC__

#include "clustering/immediate_consistency/query/master.hpp"

template <class protocol_t>
master_t<protocol_t>::master_t(
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
    read_pool(10, &read_queue, &cb),
    write_pool(10, &write_queue, &cb),
    read_mailbox(mailbox_manager, boost::bind(boost::bind(&master_t<protocol_t>::on_read,
                                              this, _1, _2, _3, _4, _5, auto_drainer_t::lock_t(&drainer)))),
    write_mailbox(mailbox_manager, boost::bind(&master_t<protocol_t>::on_write,
                                               this, _1, _2, _3, _4, _5, auto_drainer_t::lock_t(&drainer))),
    master_directory(md), master_directory_lock(mdl),
    uuid(generate_uuid()) {

    rassert(ack_checker);

    master_business_card_t<protocol_t> bcard(
        region,
        read_mailbox.get_address(), write_mailbox.get_address(),
        registrar.get_business_card());
    mutex_assertion_t::acq_t master_directory_lock_acq(master_directory_lock);
    std::map<master_id_t, master_business_card_t<protocol_t> > master_map = master_directory->get_watchable()->get();
    master_map.insert(std::make_pair(uuid, bcard));
    master_directory->set_value(master_map);
}

template <class protocol_t>
master_t<protocol_t>::~master_t() {
    mutex_assertion_t::acq_t master_directory_lock_acq(master_directory_lock);
    std::map<master_id_t, master_business_card_t<protocol_t> > master_map = master_directory->get_watchable()->get();
    master_map.erase(uuid);
    master_directory->set_value(master_map);
}

template <class protocol_t>
void master_t<protocol_t>::on_read(namespace_interface_id_t parser_id, typename protocol_t::read_t read, order_token_t otok, fifo_enforcer_read_token_t token,
        mailbox_addr_t<void(boost::variant<typename protocol_t::read_response_t, std::string>)> response_address,
        auto_drainer_t::lock_t keepalive)
        THROWS_NOTHING
{
    read_queue.push(boost::bind(&master_t<protocol_t>::perform_read,parser_id, typename protocol_t::read_t read, order_token_t otok, fifo_enforcer_read_token_t token,
        mailbox_addr_t<void(boost::variant<typename protocol_t::read_response_t, std::string>)> response_address,
        auto_drainer_t::lock_t keepalive 
}

template <class protocol_t>
void master_t<protocol_t>::perform_read(namespace_interface_id_t parser_id, typename protocol_t::read_t read, order_token_t otok, fifo_enforcer_read_token_t token,
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

template <class protocol_t>
void master_t<protocol_t>::on_write(namespace_interface_id_t parser_id, typename protocol_t::write_t write, order_token_t otok, fifo_enforcer_write_token_t token,
              mailbox_addr_t<void(boost::variant<typename protocol_t::write_response_t, std::string>)> response_address,
              auto_drainer_t::lock_t keepalive)
    THROWS_NOTHING
{
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

#endif
