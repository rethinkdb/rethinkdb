// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_MINIDIR_HPP_
#define CLUSTERING_GENERIC_MINIDIR_HPP_

#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/watchable_map.hpp"
#include "containers/archive/boost_types.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/mailbox/typed.hpp"

/* "Minidir" is short for "mini directory". The minidir classes are for broadcasting
state across the network just like the directory, but the broadcast is to a finite set of
mailboxes instead of to every connected server. This gives better performance and
sometimes better abstraction. */

typedef uuid_u minidir_link_id_t;

template<class key_t, class value_t>
class minidir_bcard_t {
public:
    /* The `minidir_write_manager_t` sends messages to the `update_mailbox_t` at the
    `minidir_read_manager_t`. It will send a series of messages with its `peer_id` and
    a randomly generated, but consistent, `link_id`; each message will have a
    `fifo_token` from the same `fifo_enforcer_source_t` created specifically for use with
    that `link_id`. `closing_link` will always be `false`; `key` and `value` will be
    set appropriately to add or remove values from the map. If the write manager is
    destroyed or decides to stop sending messages to that particular read manager, then
    it will send one final message with `closing_link` set to `true`. If contact is
    lost between them, no final message will be sent, but the read manager will erase all
    of the recorded key-value pairs from that peer. If the write manager decides to
    start sending message again, or if contact is reestablished, then it will start over
    with a new `link_id` and a new `fifo_enforcer_source_t`. */
    typedef mailbox_t<void(
        peer_id_t peer_id,
        minidir_link_id_t link_id,
        fifo_enforcer_write_token_t fifo_token,
        bool closing_link,
        boost::optional<key_t> key,
        boost::optional<value_t> value
        )> update_mailbox_t;
    typename update_mailbox_t::address_t update_mailbox;

    RDB_MAKE_ME_SERIALIZABLE_1(minidir_bcard_t, update_mailbox);
};

/* Instantiate a `minidir_read_manager_t` on the server that's supposed to be receiving
messages, and put the return value of `get_bcard()` in the directory. Then read the
received values from `get_values()`. */
template<class key_t, class value_t>
class minidir_read_manager_t {
public:
    explicit minidir_read_manager_t(mailbox_manager_t *mm);

    minidir_bcard_t<key_t, value_t> get_bcard() {
        minidir_bcard_t<key_t, value_t> bc;
        bc.update_mailbox = update_mailbox.get_address();
        return bc;
    }

    watchable_map_t<key_t, value_t> *get_values() {
        return &map_var;
    }

private:
    /* If we lose contact with a peer, we need to erase all of their entries from the
    `map_var`. This is complicated because it may later reconnect, and there are various
    race conditions. Here's how it all works:
     - When `on_update()` gets the first message from a peer, it creates an entry in
        `peer_map`. This entry contains an `auto_drainer_t::lock_t` holding the peer's
        connection, so the peer can't disconnect and reconnect until we release the lock.
     - `on_connection_change()` gets called whenever a peer connects or disconnects. We
        ignore connects, but on a disconnect we remove the entry from the map and delete
        it. The actual deletion is done in a coroutine, since destroying `drainer` may
        block.
     - Destroying the `peer_data_t` destroys all the `watchable_map_var_t::entry_t` for
        links from that peer, so all the entries disappear from the map.
     - If the peer reconnects, the process starts over from step 1.
    */
    class peer_data_t {
    public:
        class link_data_t {
        public:
            std::map<key_t, typename watchable_map_var_t<key_t, value_t>::entry_t> map;
            fifo_enforcer_sink_t fifo_sink;
        };
        explicit peer_data_t(auto_drainer_t::lock_t ck) : connection_keepalive(ck) { }
        auto_drainer_t::lock_t connection_keepalive;
        std::map<uuid_u, scoped_ptr_t<link_data_t> > link_map;
        auto_drainer_t drainer;
    };

    void on_connection_change(
        const peer_id_t &peer_id,
        const connectivity_cluster_t::connection_pair_t *pair);

    void on_update(
        signal_t *interruptor,
        const peer_id_t &peer_id,
        const minidir_link_id_t &link_id,
        fifo_enforcer_write_token_t token,
        bool closing_link,
        const boost::optional<key_t> &key,
        const boost::optional<value_t> &value);

    mailbox_manager_t *const mailbox_manager;

    watchable_map_var_t<key_t, value_t> map_var;

    std::map<peer_id_t, scoped_ptr_t<peer_data_t> > peer_map;

    /* This must be destroyed after we destroy `connection_subs`, since it may spawn
    coroutines that hold this. But it must be destroyed before we destroy the other
    member variables, since those coroutines access the other member variables. */
    auto_drainer_t drainer;

    typename minidir_bcard_t<key_t, value_t>::update_mailbox_t update_mailbox;

    watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t>::all_subs_t
        connection_subs;
};

/* Instantiate a `minidir_write_manager_t` on the server that's supposed to be sending
values. The `values` parameter to the constructor is the values to transmit, and the
`readers` parameter is where to transmit them. */
template<class reader_id_t, class key_t, class value_t>
class minidir_write_manager_t {
public:
    minidir_write_manager_t(
        mailbox_manager_t *mm,
        watchable_map_t<key_t, value_t> *_values,
        watchable_map_t<reader_id_t, minidir_bcard_t<key_t, value_t> > *_readers);
    ~minidir_write_manager_t();

private:
    /* Summary of how this works:

     - `on_reader_change()` creates and destroys `link_data_t`s. When it creates the
        first `link_data_t` for a given peer, it creates a `peer_data_t` to put it in.
        When it creates a `link_data_t` it generates a new `link_id` and calls
        `spawn_update()` for every key-value pair in `values` at that time. When it
        destroys a `link_data_t` it calls `spawn_closing_link()`. When it destroys the
        last `link_data_t` for a peer, it destroys the `peer_data_t`.

     - `on_connection_change()` destroys `peer_data_t`s for peers that have disconnected.
        This means it destroys `link_data_t`s as a side effect. When a peer reconnects,
        it calls `on_reader_change()` for every reader, to re-create the `link_data_t`s.
        So the re-created `link_data_t`s will have new `link_id`s, and they will get a
        new set of initial messages.

     - `on_value_change()` doesn't create or destroy `peer_data_t`s or `link_data_t`s; it
        just calls `spawn_update()` for every existing `link_data_t`.

    So at startup, we initialize everything by just calling `on_reader_change()` on every
    reader in the map. This takes care of setting up the `peer_data_t`s and sending the
    initial messages too. */

    class peer_data_t {
    public:
        class link_data_t {
        public:
            minidir_link_id_t link_id;
            minidir_bcard_t<key_t, value_t> bcard;
            fifo_enforcer_source_t fifo_source;
        };
        explicit peer_data_t(auto_drainer_t::lock_t ck) : connection_keepalive(ck) { }
        /* We hold `connection_keepalive` to force the `connectivity_cluster_t` to notify
        us that the old connection has been dropped, so we'll delete the lock, before it
        starts a new connection. This prevents us getting confused. */
        auto_drainer_t::lock_t connection_keepalive;
        std::map<reader_id_t, scoped_ptr_t<link_data_t> > link_map;
    };

    void on_connection_change(
        const peer_id_t &peer,
        const connectivity_cluster_t::connection_pair_t *pair);
    void on_value_change(
        const key_t &key,
        const value_t *value);
    void on_reader_change(
        const reader_id_t &reader_id,
        const minidir_bcard_t<key_t, value_t> *bcard);

    void spawn_update(
        typename peer_data_t::link_data_t *link_data,
        const key_t &key,
        const boost::optional<value_t> &value);
    void spawn_closing_link(
        typename peer_data_t::link_data_t *link_data);

    mailbox_manager_t *const mailbox_manager;
    watchable_map_t<key_t, value_t> *const values;
    watchable_map_t<reader_id_t, minidir_bcard_t<key_t, value_t> > *const readers;

    std::map<peer_id_t, scoped_ptr_t<peer_data_t> > peer_map;
    bool stopping;

    /* This must be destroyed after we destroy the `subs`es, since they may spawn new
    coroutines that hold locks on it. But it must be destroyed before the other member
    variables, since those coroutines may access the other member variables. */
    auto_drainer_t drainer;

    typename watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t>
        ::all_subs_t connection_subs;
    typename watchable_map_t<key_t, value_t>::all_subs_t value_subs;
    typename watchable_map_t<reader_id_t, minidir_bcard_t<key_t, value_t> >::all_subs_t
        reader_subs;
};

#endif /* CLUSTERING_GENERIC_MINIDIR_HPP_ */

