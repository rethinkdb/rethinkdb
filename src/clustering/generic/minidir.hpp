// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_MINIDIR_HPP_
#define CLUSTERING_GENERIC_MINIDIR_HPP_

/* "Minidir" is short for "mini directory". The minidir classes are for broadcasting
state across the network just like the directory, but the broadcast is to a finite set of
mailboxes instead of to every connected server. This gives better performance and
sometimes better abstraction. */

typedef uuid_u minidir_conn_id_t;

template<class key_t, class value_t>
class minidir_bcard_t {
public:
    /* The `minidir_write_manager_t` sends messages to the `update_mailbox_t` at the
    `minidir_read_manager_t`. It will send a series of messages with its `peer_id` and
    a randomly generated, but consistent, `conn_id`; each message will have a
    `fifo_token` from the same `fifo_enforcer_source_t` created specifically for use with
    that `conn_id`. `closing_conn` will always be `false`; `key` and `value` will be
    set appropriately to add or remove values from the map. If the write manager is
    destroyed or decides to stop sending messages to that particular read manager, then
    it will send one final message with `closing_conn` set to `true`. If contact is
    lost between them, no final message will be sent, but the read manager will erase all
    of the recorded key-value pairs from that connection. If the write manager decides to
    start sending message again, or if contact is reestablished, then it will start over
    with a new `conn_id` and a new `fifo_enforcer_source_t`. */
    typedef mailbox_t<void(
        peer_id_t peer_id,
        minidir_conn_id_t conn_id,
        fifo_enforcer_write_token_t fifo_token,
        bool closing_conn,
        key_t key,
        boost::optional<value_t> value
        )> update_mailbox_t;
    update_mailbox_t::addr_t update_mailbox;
};

template<class key_t, class value_t>
class minidir_read_manager_t {
public:
    minidir_read_manager_t(mailbox_manager_t *mm);
    ~minidir_read_manager_t() {
        stopping = true;
    }

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
     - When `on_update()` receives an update, it creates an entry in `peer_map` for the
        peer that sent that update, if there isn't already an entry.
     - The `peer_data_t` constructor creates a `disconnect_watcher_t` for that peer. This
        relies on the fact that it's impossible for the connection to be dropped and then
        reestablished while a mailbox callback is running for that peer. So the
        `disconnect_watcher_t` is guaranteed to correspond to the connection session over
        which the message arrived.
     - `on_update()` holds the `auto_drainer_t` for that `peer_data_t` while it's waiting
        to acquire `conn_data_t::fifo_sink`. When it acquires it, then it creates or
        destroys an entry in `conn_data_t::map`, and then returns.
     - When the `disconnect_watcher_t` is pulsed, it calls `peer_data_t::run()`.
     - `peer_data_t::run()` spawns an instance of `destroy_peer()` in a separate
        coroutine. It has to be a separate coroutine because it's illegal to destroy a
        signal from within its own callback.
     - `destroy_peer()` removes the entry from the `peer_map`.
     - When the `peer_data_t` is destroyed, it takes all the
        `watchable_map_var_t::entry_t`s with it, so the entries disappear from `map_var`.
     - But if `peer_data_t::run()` sees that `stopping` is set to `true`, it won't spawn
        `destroy_peer()`. This makes sure that it won't try to acquire
        `minidir_read_manager_t::drainer` while it's being destroyed. The entry in
        `peer_map` will be destroyed soon anyway.
     - Because it's impossible for the connection to be reestablished until the
        `disconnect_watcher_t` is destroyed, there's never any ambiguity about which
        connection session a given entry in `peer_map` corresponds to.
    */
    class peer_data_t : private signal_t::subscription_t {
    public:
        class conn_data_t {
        public:
            std::map<key_t, typename watchable_map_var_t<key_t, value_t>::entry_t> map;
            fifo_enforcer_sink_t fifo_sink;
        }
        peer_data_t(minidir_read_manager_t *parent, peer_id_t peer);
        void run();   /* `disconnect_watcher` calls this */
        minidir_read_manager_t *parent;
        peer_id_t peer;
        disconnect_watcher_t disconnect_watcher;
        std::map<uuid_u, conn_data_t> conn_map;
        auto_drainer_t drainer;
    };

    void destroy_peer(
        peer_id_t peer,
        auto_drainer_t::lock_t keepalive);

    void on_update(
        signal_t *interruptor,
        const peer_id_t &peer_id,
        const uuid_u &src_id,
        firo_enforcer_write_token_t token,
        bool closing_conn,
        const key_t &key,
        const boost::optional<value_t> &value);

    mailbox_manager_t *const mailbox_manager;

    watchable_map_var_t<key_t, value_t> map_var;

    bool stopping;

    /* `peer_map`'s destructor will access `map_var` by destroying the `entry_t`s. In
    addition, it may spawn instances of `peer_data_t::run()`, which access `stopping`,
    and if `stopping` is `false`, access `drainer`. So this destructor order works out as
    long as we set `stopping` to `true` in `~minidir_read_manager_t()`. */
    std::map<peer_id_t, scoped_ptr_t<peer_data_t> > peer_map;

    /* This must be destroyed before we destroy `peer_map` because instances of
    `peer_data_t::run()` may access `peer_map`. */
    auto_drainer_t drainer;

    /* This must be destroyed before `peer_map` because it spawns instances of
    `on_update()`, which access `peer_map`. */
    typename minidir_bcard_t<key_t, value_t>::update_mailbox_t update_mailbox;
};

template<class key_t, class value_t>
class minidir_write_manager_t {
public:
    minidir_write_manager_t(
        watchable_map_t<key_t, value_t> *values,
        watchable_map_t<uuid_u, minidir_bcard_t<key_t, value_t> > *readers);

private:
    class peer_data_t : private signal_t::subscription_t {
    public:
        class sink_data_t {
        public:
            minidir_conn_id_t conn_id;
            fifo_enforcer_source_t fifo_source;
        };
        peer_data_t(minidir_read_manager_t *parent, peer_id_t peer);
        void run();   /* `disconnect_watcher` calls this */
        minidir_read_manager_t *parent;
        peer_id_t peer;
        disconnect_watcher_t disconnect_watcher;
        std::map<uuid_u, sink_data_t> sink_map;
    };
};

#endif /* CLUSTERING_GENERIC_MINIDIR_HPP_ */

