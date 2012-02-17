#ifndef __RPC_DIRECTORY_MANAGER_HPP__
#define __RPC_DIRECTORY_MANAGER_HPP__

#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/view.hpp"
#include "rpc/directory/write_manager.hpp"

template<class metadata_t>
class directory_readwrite_manager_t :
    public directory_read_manager_t<metadata_t>,
    public directory_write_manager_t<metadata_t>,
    public directory_readwrite_service_t
{
public:
    directory_readwrite_manager_t(
        message_service_t *super,
        const metadata_t &initial_metadata) THROWS_NOTHING;
    ~directory_readwrite_manager_t() THROWS_NOTHING { }

    clone_ptr_t<directory_rwview_t<metadata_t> > get_root_view() THROWS_NOTHING;

private:
    class root_view_t : public directory_rwview_t<metadata_t> {
    public:
        root_view_t *clone() const THROWS_NOTHING;
        directory_readwrite_service_t *get_directory_service() THROWS_NOTHING;
        boost::optional<metadata_t> get_value(peer_id_t peer) THROWS_NOTHING;
        metadata_t get_our_value(directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING;
        void set_our_value(const metadata_t &new_value_for_us, directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING;
    private:
        friend class directory_readwrite_manager_t;
        explicit root_view_t(directory_readwrite_manager_t *p) THROWS_NOTHING;
        directory_readwrite_manager_t *const parent;
        const clone_ptr_t<directory_rview_t<metadata_t> > read_view;
        const clone_ptr_t<directory_wview_t<metadata_t> > write_view;
    };
};

#include "rpc/directory/manager.tcc"

#endif /* __RPC_DIRECTORY_MANAGER_HPP__ */
