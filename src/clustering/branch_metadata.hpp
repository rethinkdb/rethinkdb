

template<class protocol_t>
struct branch_metadata_t {

    /* The initial state of the branch is composed of these regions of these
    versions. */

    std::vector<std::pair<typename protocol_t::region_t, version_t> > parents;

    /* If `alive` is `false`, then the branch is defunct, the master no longer
    exists, and all the master-mailboxes are nil. */

    bool alive;

    /* These are the mailboxes to send reads and writes to if you want to
    operate on the branch. */

    typedef async_mailbox_t<void(
        typename protocol_t::read_t, order_token_t,
        async_mailbox_t<void(typename protocol_t::read_response_t)>::address_t
        )> read_mailbox_t;
    read_mailbox_t::address_t read_mailbox;

    typedef async_mailbox_t<void(
        typename protocol_t::write_t, order_token_t,
        async_mailbox_t<void(typename protocol_t::write_response_t)>::address_t
        )> write_mailbox_t;
    write_mailbox_t::address_t write_mailbox;

    /* These are the mirrors that the branch has. */

    std::map<mirror_id_t, mirror_metadata_t> mirrors;

    /* When a mirror first starts up, it sends a message to `join_mailbox` to
    start receiving writes. */

    typedef async_mailbox_t<void(
        typename protocol_t::write_t, repli_timestamp_t, order_token_t,
        async_mailbox_t<void(typename protocol_t::write_response_t)>::address_t
        )> mirror_write_mailbox_t;

    typedef async_mailbox_t<void(
        )> mirror_greet_mailbox_t;

    typedef async_mailbox_t<void(
        mirror_id_t,
        mirror_greet_mailbox_t::address_t,
        mirror_write_mailbox_t::address_t
        )> join_mailbox_t;
    join_mailbox_t::address_t join_mailbox;

    /* When a mirror is done backfilling, it sends a message to `ready_mailbox`
    to start receiving reads. */

    typedef async_mailbox_t<void(
        typename protocol_t::read_t, order_token_t,
        async_mailbox_t<void(typename protocol_t::read_response_t)>::address_t
        )> mirror_read_mailbox_t;

    typedef async_mailbox_t<void(
        mirror_id_t,
        mirror_read_mailbox_t::address_t
        )> ready_mailbox_t;
    ready_mailbox_t::address_t ready_mailbox;

    /* When it shuts down, it sends a message to `leave_mailbox` to indicate
    it's done. If the master ever loses sight of a node, it acts as though a
    message was sent to `leave_mailbox`. */

    typedef async_mailbox_t<void(
        mirror_id_t
        )> leave_mailbox_t;
    leave_mailbox_t::address_t leave_mailbox;
};

typedef boost::uuids::uuid branch_id_t;

struct 