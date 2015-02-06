// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_MINIDIR_HPP_
#define CLUSTERING_GENERIC_MINIDIR_HPP_

/* "Minidir" is short for "mini directory". The minidir classes are for broadcasting
state across the network just like the directory, but the broadcast is to a finite set of
mailboxes instead of to every connected server. This gives better performance and
sometimes better abstraction. */

template<class key_t, class value_t>
class minidir_bcard_t {
public:
    typedef mailbox_t<void(peer_id_t, key_t, boost::optional<value_t>)> update_mailbox_t;
    update_mailbox_t::addr_t update_mailbox;
};

template<class key_t, class value_t>
class minidir_read_manager_t {
public:
    minidir_bcard_t<key_t, value_t> get_bcard();

    watchable_map_t<key_t, value_t> *get_values();
};

template<class key_t, class value_t>
class minidir_write_manager_t {
public:
    minidir_write_manager_t(
        watchable_map_t<key_t, value_t> *values,
        watchable_map_t<uuid_u, minidir_bcard_t<key_t, value_t> > *readers);
};

#endif /* CLUSTERING_GENERIC_MINIDIR_HPP_ */

