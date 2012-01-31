#ifndef __RPC_DIRECTORY_WRITE_MANAGER_HPP__
#define __RPC_DIRECTORY_WRITE_MANAGER_HPP__

#include "rpc/connectivity/connectivity.hpp"
#include "rpc/connectivity/messages.hpp"
#include "rpc/directory/write_view.hpp"

template<class metadata_t>
class directory_write_manager_t :
    public virtual directory_write_service_t
{
public:
    directory_write_manager_t(
        message_service_t *message_service,
        const metadata_t &initial_metadata) THROWS_NOTHING;

    clone_ptr_t<directory_wview_t<metadata_t> > get_root_view() THROWS_NOTHING;

private:
    class root_view_t : public directory_wview_t<metadata_t> {
    public:
        root_view_t *clone();
        metadata_t get_our_value(directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING;
        void set_our_value(const metadata_t &new_value_for_us, directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING;
        directory_write_service_t *get_directory_service() THROWS_NOTHING;
    private:
        friend class directory_write_manager_t;
        explicit root_view_t(directory_write_manager_t *) THROWS_NOTHING;
        directory_write_manager_t *const parent;
    };

    void on_connect(peer_id_t peer) THROWS_NOTHING;

    void send_initialization(peer_id_t peer, const metadata_t &initial_value, fifo_enforcer_source_t::state_t metadata_fifo_state, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;
    void send_update(peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    static void write_initialization(std::ostream &os, const metadata_t &initial_value, fifo_enforcer_source_t::state_t metadata_fifo_state) THROWS_NOTHING;
    static void write_update(std::ostream &os, const metadata_t &new_value, fifo_enforcer_write_token_t metadata_fifo_token) THROWS_NOTHING;

    /* `directory_write_service_t` interface: */
    mutex_t *get_our_value_lock() THROWS_NOTHING;

    message_service_t *const message_service;

    mutex_t our_value_lock;

    metadata_t our_value;
    fifo_enforcer_source_t metadata_fifo_source;

    auto_drainer_t drainer;

    connectivity_service_t::peers_list_subscription_t connectivity_subscription;
};

#include "rpc/directory/write_manager.tcc"

#endif /* __RPC_DIRECTORY_WRITE_MANAGER_HPP__ */
