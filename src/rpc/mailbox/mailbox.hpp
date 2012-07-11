#ifndef RPC_MAILBOX_MAILBOX_HPP_
#define RPC_MAILBOX_MAILBOX_HPP_

#include "errors.hpp"
#include <boost/function.hpp>

#include "containers/archive/archive.hpp"
#include "rpc/connectivity/cluster.hpp"

struct mailbox_manager_t;

/* `mailbox_t` is a receiver of messages. Construct it with a callback function
to handle messages it receives. To send messages to the mailbox, call the
`get_address()` method and then call `send()` on the address it returns. */

struct raw_mailbox_t :
    public home_thread_mixin_t
{
private:
    friend struct mailbox_manager_t;

    mailbox_manager_t *manager;

    typedef int id_t;
    id_t mailbox_id;

    boost::function<void(read_stream_t *)> callback;

    auto_drainer_t drainer;

    DISABLE_COPYING(raw_mailbox_t);

public:
    struct address_t {

        /* Constructs a nil address */
        address_t();

        address_t(const address_t&);

        /* Tests if the address is nil */
        bool is_nil() const;

        /* Returns the peer on which the mailbox lives. If the address is nil,
        fails. */
        peer_id_t get_peer() const;

        // Returns a friendly human-readable peer:thread:mailbox_id string.
        std::string human_readable() const;

    private:
        friend void send(mailbox_manager_t *, raw_mailbox_t::address_t, boost::function<void(write_stream_t *)>);
        friend struct raw_mailbox_t;
        friend struct mailbox_manager_t;

        RDB_MAKE_ME_SERIALIZABLE_3(peer, thread, mailbox_id);

        /* The peer on which the mailbox is located */
        peer_id_t peer;

        /* The thread on `peer` that the mailbox lives on */
        int thread;

        /* The ID of the mailbox */
        id_t mailbox_id;
    };

    raw_mailbox_t(mailbox_manager_t *, const boost::function<void(read_stream_t *)> &);
    ~raw_mailbox_t();

    address_t get_address();
};

/* `send()` sends a message to a mailbox. It is safe to call `send()` outside of
a coroutine; it does not block. If the mailbox does not exist or the peer is
inaccessible, `send()` will silently fail. */

void send(
    mailbox_manager_t *src,
    raw_mailbox_t::address_t dest,
    boost::function<void(write_stream_t *)> message
    );

/* `mailbox_manager_t` uses a `message_service_t` to provide mailbox capability.
Usually you will split a `message_service_t` into several sub-services using
`message_multiplexer_t` and put a `mailbox_manager_t` on only one of them,
because the `mailbox_manager_t` relies on something else to send the initial
mailbox addresses back and forth between nodes. */

struct mailbox_manager_t : public message_handler_t {

public:
    explicit mailbox_manager_t(message_service_t *);

    /* Returns the connectivity service of the underlying message service. */
    connectivity_service_t *get_connectivity_service() {
        return message_service->get_connectivity_service();
    }

private:
    friend struct raw_mailbox_t;
    friend void send(mailbox_manager_t *, raw_mailbox_t::address_t, boost::function<void(write_stream_t *)>);

    message_service_t *message_service;

    struct mailbox_table_t {
        mailbox_table_t();
        ~mailbox_table_t();
        raw_mailbox_t::id_t next_mailbox_id;
        std::map<raw_mailbox_t::id_t, raw_mailbox_t *> mailboxes;
        raw_mailbox_t *find_mailbox(raw_mailbox_t::id_t);
    };
    one_per_thread_t<mailbox_table_t> mailbox_tables;

    static void write_mailbox_message(write_stream_t *stream, int dest_thread, raw_mailbox_t::id_t dest_mailbox_id, boost::function<void(write_stream_t *)> writer);
    void on_message(peer_id_t, read_stream_t *stream);
};

#endif /* RPC_MAILBOX_MAILBOX_HPP_ */
