#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_

#include <map>

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/query/metadata.hpp"

template<class protocol_t>
class master_t {
public:
    master_t(
            mailbox_manager_t *mm,
            watchable_variable_t<std::map<master_id_t, master_business_card_t<protocol_t> > > *md,
            mutex_assertion_t *mdl,
            typename protocol_t::region_t region,
            broadcaster_t<protocol_t> *b)
            THROWS_ONLY(interrupted_exc_t) :
        mailbox_manager(mm),
        broadcaster(b),
        read_mailbox(mailbox_manager, boost::bind(&master_t<protocol_t>::on_read,
                                                  this, _1, _2, _3, _4, _5, auto_drainer_t::lock_t(&drainer))),
        write_mailbox(mailbox_manager, boost::bind(&master_t<protocol_t>::on_write,
                                                   this, _1, _2, _3, _4, _5, auto_drainer_t::lock_t(&drainer))),
        registrar(mm, this),
        master_directory(md), master_directory_lock(mdl),
        uuid(generate_uuid()) {

        master_business_card_t<protocol_t> bcard(
            region,
            read_mailbox.get_address(), write_mailbox.get_address(),
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
        keepalive.assert_is_holding(&drainer);
        try {
            typename std::map<namespace_interface_id_t, parser_lifetime_t *>::iterator it = sink_map.find(parser_id);
            // TODO: Remove this assertion.  Out-of-order operations (which allegedly can happen) could cause it to be wrong?
            rassert(it != sink_map.end());

            typename protocol_t::read_response_t response;
            {
                auto_drainer_t::lock_t auto_drainer_lock(it->second->drainer());
                fifo_enforcer_sink_t::exit_read_t exiter(it->second->sink(), token);
                response = broadcaster->read(read, &exiter, otok);
            }
            send(mailbox_manager, response_address, boost::variant<typename protocol_t::read_response_t, std::string>(response));
        } catch (typename broadcaster_t<protocol_t>::mirror_lost_exc_t e) {
            send(mailbox_manager, response_address, boost::variant<typename protocol_t::read_response_t, std::string>(std::string(e.what())));
        } catch (typename broadcaster_t<protocol_t>::insufficient_mirrors_exc_t e) {
            send(mailbox_manager, response_address, boost::variant<typename protocol_t::read_response_t, std::string>(std::string(e.what())));
        }
    }

    void on_write(namespace_interface_id_t parser_id, typename protocol_t::write_t write, order_token_t otok, fifo_enforcer_write_token_t token,
                  mailbox_addr_t<void(boost::variant<typename protocol_t::write_response_t, std::string>)> response_address,
                  auto_drainer_t::lock_t keepalive)
        THROWS_NOTHING
    {
        keepalive.assert_is_holding(&drainer);
        try {
            typename std::map<namespace_interface_id_t, parser_lifetime_t *>::iterator it = sink_map.find(parser_id);
            // TODO: Remove this assertion.  Out-of-order operations (which allegedly can hoppen) could cause it to be wrong?
            rassert(it != sink_map.end());

            typename protocol_t::write_response_t response;
            {
                auto_drainer_t::lock_t auto_drainer_lock(it->second->drainer());
                fifo_enforcer_sink_t::exit_write_t exiter(it->second->sink(), token);
                response = broadcaster->write(write, &exiter, otok);
            }
            send(mailbox_manager, response_address, boost::variant<typename protocol_t::write_response_t, std::string>(response));
        } catch (typename broadcaster_t<protocol_t>::mirror_lost_exc_t e) {
            send(mailbox_manager, response_address, boost::variant<typename protocol_t::write_response_t, std::string>(std::string(e.what())));
        } catch (typename broadcaster_t<protocol_t>::insufficient_mirrors_exc_t e) {
            send(mailbox_manager, response_address, boost::variant<typename protocol_t::write_response_t, std::string>(std::string(e.what())));
        }
    }

    mailbox_manager_t *mailbox_manager;
    broadcaster_t<protocol_t> *broadcaster;
    std::map<namespace_interface_id_t, parser_lifetime_t *> sink_map;
    auto_drainer_t drainer;

    typename master_business_card_t<protocol_t>::read_mailbox_t read_mailbox;
    typename master_business_card_t<protocol_t>::write_mailbox_t write_mailbox;

    registrar_t<namespace_interface_business_card_t, master_t *, parser_lifetime_t> registrar;

    watchable_variable_t<std::map<master_id_t, master_business_card_t<protocol_t> > > *master_directory;
    mutex_assertion_t *master_directory_lock;
    master_id_t uuid;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_MASTER_HPP_ */
