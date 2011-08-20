#ifndef __RPC_MAILBOX_MAILBOX_HPP__
#define __RPC_MAILBOX_MAILBOX_HPP__

#include "rpc/connectivity/connectivity.hpp"

class mailbox_cluster_t;

/* `mailbox_t` is a receiver of messages. Instantiate a subclass that overrides
`on_message()` to handle messages you receive. To send messages to the mailbox,
construct a `mailbox_t::address_t` with this mailbox and then call `send()` on
the address. */

struct mailbox_t :
    public home_thread_mixin_t
{
private:
    friend class mailbox_cluster_t;

    mailbox_cluster_t *cluster;

    typedef int id_t;
    id_t mailbox_id;

    DISABLE_COPYING(mailbox_t);

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

    private:
        friend void send(mailbox_cluster_t *, mailbox_t::address_t, boost::function<void(std::ostream&)>);
        friend class mailbox_t;

        /* The peer on which the mailbox is located */
        peer_id_t peer;

        /* The thread on `peer` that the mailbox lives on */
        int thread;

        /* The ID of the mailbox */
        id_t mailbox_id;
    };

    address_t get_address();

protected:
    mailbox_t(mailbox_cluster_t *);
    virtual ~mailbox_t();

    /* Called to when a message is sent to the mailbox over the network. Should
    deserialize the message, then call the second argument to indicate it is
    done deserializing, then act on the message. */
    virtual void on_message(std::istream&, boost::function<void()>&) = 0;
};

/* `send()` sends a message to a mailbox. It is safe to call `send()` outside of
a coroutine; it does not block. If the mailbox does not exist or the peer is
inaccessible, `send()` will silently fail. */

void send(
    mailbox_cluster_t *src,
    mailbox_t::address_t dest,
    boost::function<void(std::ostream&)> message
    );

/* `mailbox_cluster_t` is a subclass of `connectivity_cluster_t` that adds
message routing infrastructure. */

struct mailbox_cluster_t : public connectivity_cluster_t {

public:
    mailbox_cluster_t(int port);
    ~mailbox_cluster_t();

protected:
    /* It's impossible to send a message to a mailbox without having its
    address, and it's impossible to transfer its address from another machine
    without sending a message. `send_utility_message()` is a way of
    "bootstrapping" the system. It sends a message directly to another peer;
    the other peer's `on_utility_message()` method will be called when the
    message arrives. The semantics are the same as with
    `connectivity_cluster_t`'s `send_message()`/`on_message()`. */
    void send_utility_message(peer_id_t, boost::function<void(std::ostream&)>);
    virtual void on_utility_message(peer_id_t, std::istream&, boost::function<void()>&) = 0;

private:
    friend class mailbox_t;
    friend void send(mailbox_cluster_t *, mailbox_t::address_t, boost::function<void(std::ostream&)>);

    struct mailbox_table_t {
        mailbox_table_t();
        ~mailbox_table_t();
        mailbox_t::id_t next_mailbox_id;
        std::map<mailbox_t::id_t, mailbox_t*> mailboxes;
        mailbox_t *find_mailbox(mailbox_t::id_t);
    };
    std::vector<mailbox_table_t> mailbox_tables;

    static void write_utility_message(std::ostream&, boost::function<void(std::ostream&)> writer);
    static void write_mailbox_message(std::ostream&, int dest_thread, mailbox_t::id_t dest_mailbox_id, boost::function<void(std::ostream&)> writer);
    void on_message(peer_id_t, std::istream&, boost::function<void()>&);
};

#endif /* __RPC_MAILBOX_MAILBOX_HPP__ */
