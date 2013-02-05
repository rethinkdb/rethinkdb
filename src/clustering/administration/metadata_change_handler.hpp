// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_METADATA_CHANGE_HANDLER_HPP_
#define CLUSTERING_ADMINISTRATION_METADATA_CHANGE_HANDLER_HPP_

#include <set>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "rpc/semilattice/view.hpp"
#include "clustering/generic/resource.hpp"
#include "concurrency/auto_drainer.hpp"

template <class metadata_t>
class metadata_change_handler_t {
public:
    typedef mailbox_t<void(bool)> result_mailbox_t;
    typedef mailbox_t<void(bool, metadata_t, typename result_mailbox_t::address_t)> commit_mailbox_t;
    typedef mailbox_t<void(metadata_t, typename commit_mailbox_t::address_t)> ack_mailbox_t;
    typedef mailbox_t<void(typename ack_mailbox_t::address_t)> request_mailbox_t;

    metadata_change_handler_t(mailbox_manager_t *_mailbox_manager,
                              const boost::shared_ptr<semilattice_readwrite_view_t<metadata_t> > &_metadata) :
        mailbox_manager(_mailbox_manager),
        request_mailbox(mailbox_manager,
                        boost::bind(&metadata_change_handler_t<metadata_t>::remote_change_request, this, _1),
                        mailbox_callback_mode_inline),
        metadata_view(_metadata)
    { }

    ~metadata_change_handler_t() {
        guarantee(coro_invalid_conditions.empty());
    }

    // Mailbox to be used for remote changes
    typename request_mailbox_t::address_t get_request_mailbox_address() {
        return request_mailbox.get_address();
    }

    // Functions used to operate on local metadata
    metadata_t get() {
        return metadata_view->get();
    }

    void update(const metadata_t& metadata) {
        for (std::set<cond_t*>::iterator i = coro_invalid_conditions.begin(); i != coro_invalid_conditions.end(); ++i) {
            (*i)->pulse_if_not_already_pulsed();
        }

        metadata_view->join(metadata);
    }

    // Object used to operate on a peer's metadata
    class metadata_change_request_t {
    public:
        metadata_change_request_t(mailbox_manager_t *_mailbox_manager,
                                  typename request_mailbox_t::address_t _request_mailbox) :
            mailbox_manager(_mailbox_manager),
            interest_acquired(true)
        {
            cond_t done;
            ack_mailbox_t ack_mailbox(mailbox_manager,
                                      boost::bind(&metadata_change_handler_t::metadata_change_request_t::handle_ack,
                                                  this, &done, _1, _2),
                                      mailbox_callback_mode_inline);

            send(mailbox_manager, _request_mailbox, ack_mailbox.get_address());
            disconnect_watcher_t dc_watcher(mailbox_manager->get_connectivity_service(), _request_mailbox.get_peer());
            wait_any_t waiter(&done, &dc_watcher);
            waiter.wait();

            if (dc_watcher.is_pulsed())
                throw resource_lost_exc_t();
        }

        ~metadata_change_request_t() {
            // If a change was never applied, notify the peer that it is no longer needed
            if (interest_acquired) {
                send(mailbox_manager, commit_mailbox_address, false, metadata_t(), result_mailbox_t::address_t());
            }
        }

        metadata_t get() const {
            return remote_metadata;
        }

        bool update(const metadata_t& metadata) {
            interest_acquired = false;
            promise_t<bool> result_promise;
            result_mailbox_t result_mailbox(mailbox_manager,
                                            boost::bind(&promise_t<bool>::pulse,
                                                        &result_promise, _1),
                                            mailbox_callback_mode_inline);

            send(mailbox_manager, commit_mailbox_address, true, metadata, result_mailbox.get_address());
            disconnect_watcher_t dc_watcher(mailbox_manager->get_connectivity_service(), commit_mailbox_address.get_peer());
            wait_any_t waiter(result_promise.get_ready_signal(), &dc_watcher);
            waiter.wait();

            bool result;
            if (result_promise.try_get_value(&result)) {
                return result;
            }
            return false;
        }

    private:
        void handle_ack(cond_t *done,
                        metadata_t& metadata,
                        typename commit_mailbox_t::address_t _commit_mailbox_address) {
            commit_mailbox_address = _commit_mailbox_address;
            remote_metadata = metadata;
            done->pulse();
        }

        mailbox_manager_t *mailbox_manager;
        bool interest_acquired;
        metadata_t remote_metadata;
        typename commit_mailbox_t::address_t commit_mailbox_address;
    };

private:
    mailbox_manager_t *mailbox_manager;
    request_mailbox_t request_mailbox;
    boost::shared_ptr<semilattice_readwrite_view_t<metadata_t> > metadata_view;
    std::set<cond_t*> coro_invalid_conditions;
    auto_drainer_t drainer;

    void remote_change_request(typename ack_mailbox_t::address_t ack_mailbox) {
        // Spawn a coroutine to wait for the metadata change
        coro_t::spawn_sometime(boost::bind(&metadata_change_handler_t::remote_change_request_coro,
                                           this,
                                           ack_mailbox,
                                           auto_drainer_t::lock_t(&drainer)));
    }

    void remote_change_request_coro(typename ack_mailbox_t::address_t ack_mailbox,
                                    auto_drainer_t::lock_t lock UNUSED) {
        cond_t invalid_condition;
        cond_t commit_done;
        commit_mailbox_t commit_mailbox(mailbox_manager,
                                        boost::bind(&metadata_change_handler_t<metadata_t>::handle_commit,
                                                    this, &commit_done, &invalid_condition, _1, _2, _3),
                                        mailbox_callback_mode_inline);

        coro_invalid_conditions.insert(&invalid_condition);

        send(mailbox_manager, ack_mailbox, metadata_view->get(), commit_mailbox.get_address());
        disconnect_watcher_t dc_watcher(mailbox_manager->get_connectivity_service(), ack_mailbox.get_peer());
        wait_any_t waiter(&commit_done, &dc_watcher);
        waiter.wait();

        guarantee(commit_done.is_pulsed() || dc_watcher.is_pulsed());
        coro_invalid_conditions.erase(&invalid_condition);
    }

    void handle_commit(cond_t *done,
                       const cond_t *invalid_condition,
                       bool commit,
                       metadata_t metadata,
                       typename result_mailbox_t::address_t result_mailbox) {
        // The other side may abandon their change request, in which case we do nothing
        if (commit) {
            bool success = !invalid_condition->is_pulsed();
            if (success) {
                update(metadata);
            }
            coro_t::spawn_sometime(boost::bind(&metadata_change_handler_t::send_result,
                                               this,
                                               success,
                                               result_mailbox,
                                               auto_drainer_t::lock_t(&drainer)));
        }
        done->pulse();
    }

    void send_result(bool result, typename result_mailbox_t::address_t result_mailbox, auto_drainer_t::lock_t lock UNUSED) {
        send<bool>(mailbox_manager, result_mailbox, result);
    }
};

#endif  // CLUSTERING_ADMINISTRATION_METADATA_CHANGE_HANDLER_HPP_
