// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_MAILBOX_MAILBOX_HPP_
#define RPC_MAILBOX_MAILBOX_HPP_

#include <map>
#include <string>

#include "containers/archive/archive.hpp"
#include "rpc/connectivity/cluster.hpp"

class mailbox_manager_t;

/* `mailbox_t` is a receiver of messages. Construct it with a callback function
to handle messages it receives. To send messages to the mailbox, call the
`get_address()` method and then call `send()` on the address it returns. */

class mailbox_write_callback_t {
public:
    virtual ~mailbox_write_callback_t() { }
    virtual void write(write_stream_t *stream) = 0;
};

class mailbox_read_callback_t {
public:
    virtual ~mailbox_read_callback_t() { }

    virtual void read(read_stream_t *stream) = 0;
};

struct raw_mailbox_t : public home_thread_mixin_t {
public:
    struct address_t;
    typedef uint64_t id_t;

private:
    friend class mailbox_manager_t;
    friend class raw_mailbox_writer_t;
    friend void send(mailbox_manager_t *, address_t, mailbox_write_callback_t *);

    mailbox_manager_t *manager;

    const id_t mailbox_id;

    mailbox_read_callback_t *callback;

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
        friend void send(mailbox_manager_t *, raw_mailbox_t::address_t, mailbox_write_callback_t *callback);
        friend struct raw_mailbox_t;
        friend class mailbox_manager_t;

        RDB_MAKE_ME_SERIALIZABLE_3(peer, thread, mailbox_id);

        /* The peer on which the mailbox is located */
        peer_id_t peer;

        /* The thread on `peer` that the mailbox lives on */
        static const int32_t ANY_THREAD;
        int32_t thread;

        /* The ID of the mailbox */
        id_t mailbox_id;
    };

    raw_mailbox_t(mailbox_manager_t *, mailbox_read_callback_t *callback);
    ~raw_mailbox_t();

    address_t get_address() const;
};

/* `send()` sends a message to a mailbox. It is safe to call `send()` outside of
a coroutine; it does not block. If the mailbox does not exist or the peer is
inaccessible, `send()` will silently fail. */

void send(mailbox_manager_t *src,
          raw_mailbox_t::address_t dest,
          mailbox_write_callback_t *callback);

/* `mailbox_manager_t` uses a `message_service_t` to provide mailbox capability.
Usually you will split a `message_service_t` into several sub-services using
`message_multiplexer_t` and put a `mailbox_manager_t` on only one of them,
because the `mailbox_manager_t` relies on something else to send the initial
mailbox addresses back and forth between nodes. */

class mailbox_manager_t : public message_handler_t {
public:
    explicit mailbox_manager_t(message_service_t *);

    /* Returns the connectivity service of the underlying message service. */
    connectivity_service_t *get_connectivity_service() {
        return message_service->get_connectivity_service();
    }

private:
    friend struct raw_mailbox_t;
    friend void send(mailbox_manager_t *, raw_mailbox_t::address_t, mailbox_write_callback_t *callback);

    message_service_t *message_service;

    struct mailbox_table_t {
        mailbox_table_t();
        ~mailbox_table_t();
        raw_mailbox_t::id_t next_mailbox_id;
        // TODO: use a buffered structure to reduce dynamic allocation
        std::map<raw_mailbox_t::id_t, raw_mailbox_t *> mailboxes;
        raw_mailbox_t *find_mailbox(raw_mailbox_t::id_t);
    };
    one_per_thread_t<mailbox_table_t> mailbox_tables;

    raw_mailbox_t::id_t generate_mailbox_id();

    raw_mailbox_t::id_t register_mailbox(raw_mailbox_t *mb);
    void unregister_mailbox(raw_mailbox_t::id_t id);

    static void write_mailbox_message(write_stream_t *stream,
                                      threadnum_t dest_thread,
                                      raw_mailbox_t::id_t dest_mailbox_id,
                                      mailbox_write_callback_t *callback);

    void on_message(peer_id_t, string_read_stream_t *stream);

    void mailbox_read_coroutine(threadnum_t dest_thread,
                                raw_mailbox_t::id_t dest_mailbox_id,
                                string_read_stream_t *stream);
};

#endif /* RPC_MAILBOX_MAILBOX_HPP_ */
