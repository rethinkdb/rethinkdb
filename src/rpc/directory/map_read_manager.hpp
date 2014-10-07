// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_DIRECTORY_MAP_READ_MANAGER_HPP_
#define RPC_DIRECTORY_MAP_READ_MANAGER_HPP_

#include "concurrency/auto_drainer.hpp"
#include "concurrency/one_per_thread.hpp"
#include "concurrency/watchable_map.hpp"
#include "rpc/connectivity/cluster.hpp"

template<class key_t, class value_t>
class directory_map_read_manager_t :
    public home_thread_mixin_t,
    public cluster_message_handler_t {
public:
    directory_map_read_manager_t(
        connectivity_cluster_t *cm,
        connectivity_cluster_t::message_tag_t tag);
    ~directory_map_read_manager_t();

    watchable_map_t<std::pair<peer_id_t, key_t>, value_t> *get_root_view() {
        return &map_var;
    }

private:
    void on_message(
            connectivity_cluster_t::connection_t *connection,
            auto_drainer_t::lock_t connection_keepalive,
            read_stream_t *stream)
            THROWS_ONLY(fake_archive_exc_t);
    void do_update(
            peer_id_t peer_id,
            auto_drainer_t::lock_t connection_keepalive,
            auto_drainer_t::lock_t this_keepalive,
            uint64_t timestamp,
            const key_t &key,
            const boost::optional<value_t> &value);

    watchable_map_var_t<std::pair<peer_id_t, key_t>, value_t> map_var;
    std::map<peer_id_t, std::map<key_t, uint64_t> > timestamps;

    /* Instances of `do_update()` hold a lock on one of these drainers. */
    one_per_thread_t<auto_drainer_t> per_thread_drainers;
};

#endif   /* RPC_DIRECTORY_MAP_READ_MANAGER_HPP_ */

