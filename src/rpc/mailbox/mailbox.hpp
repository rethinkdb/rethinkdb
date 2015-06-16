// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_MAILBOX_MAILBOX_HPP_
#define RPC_MAILBOX_MAILBOX_HPP_

#include <map>
#include <string>
#include <vector>

#include "backtrace.hpp"
#include "concurrency/new_semaphore.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/vector_stream.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/semilattice/joins/macros.hpp"

class mailbox_manager_t;

/* `mailbox_t` is a receiver of messages. Construct it with a callback function
to handle messages it receives. To send messages to the mailbox, call the
`get_address()` method and then call `send()` on the address it returns. */

class mailbox_write_callback_t {
public:
    virtual ~mailbox_write_callback_t() { }
    virtual void write(cluster_version_t cluster_version,
                       write_message_t *wm) = 0;
#ifdef ENABLE_MESSAGE_PROFILER
    virtual const char *message_profiler_tag() const = 0;
#endif
};

class mailbox_read_callback_t {
public:
    virtual ~mailbox_read_callback_t() { }

    /* `read()` is allowed to block indefinitely after reading the message from the
    stream, but the mailbox's destructor cannot return until `read()` returns. */
    virtual void read(
        read_stream_t *stream,
        /* `interruptor` will be pulsed if the mailbox is destroyed. */
        signal_t *interruptor) = 0;
};

struct raw_mailbox_t : public home_thread_mixin_t {
public:
    struct address_t;
    typedef uint64_t id_t;

#ifndef NDEBUG
    lazy_backtrace_formatter_t bt;
#endif

private:
    friend class mailbox_manager_t;
    friend class raw_mailbox_writer_t;
    friend void send(mailbox_manager_t *, address_t, mailbox_write_callback_t *);

    mailbox_manager_t *manager;

    const id_t mailbox_id;

    /* `callback` will be set to `nullptr` after `begin_shutdown()` is called. This is
    both a way of ensuring that no new callbacks are spawned and of making sure that
    the destructor won't call `begin_shutdown()` again. */
    mailbox_read_callback_t *callback;

    auto_drainer_t drainer;

    DISABLE_COPYING(raw_mailbox_t);

public:
    struct address_t {
        bool operator<(const address_t &other) const {
            return peer != other.peer
                ? peer < other.peer
                : (thread != other.thread
                   ? thread < other.thread
                   : mailbox_id < other.mailbox_id);
        }

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

        RDB_MAKE_ME_EQUALITY_COMPARABLE_3(raw_mailbox_t::address_t, peer, thread, mailbox_id);

        RDB_MAKE_ME_SERIALIZABLE_3(address_t, peer, thread, mailbox_id);

    private:
        friend void send(mailbox_manager_t *, raw_mailbox_t::address_t, mailbox_write_callback_t *callback);
        friend struct raw_mailbox_t;
        friend class mailbox_manager_t;

        /* The peer on which the mailbox is located */
        peer_id_t peer;

        /* The thread on `peer` that the mailbox lives on */
        int32_t thread;

        /* The ID of the mailbox */
        id_t mailbox_id;
    };

    raw_mailbox_t(mailbox_manager_t *, mailbox_read_callback_t *callback);

    /* Note that `~raw_mailbox_t()` will block until all of the callbacks have finished
    running. */
    ~raw_mailbox_t();

    /* `begin_shutdown()` will stop the mailbox from accepting further queries, and
    pulse the interruptor for every running instance of the callback. */
    void begin_shutdown();

    address_t get_address() const;
};

/* `send()` sends a message to a mailbox. `send()` can block and must be called
in a coroutine. If the mailbox does not exist or the peer is disconnected, `send()`
will silently fail. */

void send(mailbox_manager_t *src,
          raw_mailbox_t::address_t dest,
          mailbox_write_callback_t *callback);

/* `mailbox_manager_t` is a `cluster_message_handler_t` that takes care
of actually routing messages to mailboxes. */

class mailbox_manager_t : public cluster_message_handler_t {
public:
    mailbox_manager_t(connectivity_cluster_t *connectivity_cluster,
                      connectivity_cluster_t::message_tag_t message_tag);

private:
    friend struct raw_mailbox_t;
    friend void send(mailbox_manager_t *, raw_mailbox_t::address_t, mailbox_write_callback_t *callback);

    struct mailbox_table_t {
        mailbox_table_t();
        ~mailbox_table_t();
        raw_mailbox_t::id_t next_mailbox_id;
        // TODO: use a buffered structure to reduce dynamic allocation
        std::map<raw_mailbox_t::id_t, raw_mailbox_t *> mailboxes;
        raw_mailbox_t *find_mailbox(raw_mailbox_t::id_t);
    };
    one_per_thread_t<mailbox_table_t> mailbox_tables;

    /* We must acquire one of these semaphores whenever we want to send a message over a
    mailbox. This prevents mailbox messages from starving directory and semilattice
    messages. */
    one_per_thread_t<new_semaphore_t> semaphores;

    raw_mailbox_t::id_t generate_mailbox_id();

    raw_mailbox_t::id_t register_mailbox(raw_mailbox_t *mb);
    void unregister_mailbox(raw_mailbox_t::id_t id);

    static void write_mailbox_message(write_stream_t *stream,
                                      threadnum_t dest_thread,
                                      raw_mailbox_t::id_t dest_mailbox_id,
                                      mailbox_write_callback_t *callback);

    void on_message(connectivity_cluster_t::connection_t *connection,
                    auto_drainer_t::lock_t connection_keeepalive,
                    read_stream_t *stream);
    void on_local_message(connectivity_cluster_t::connection_t *connection,
                          auto_drainer_t::lock_t connection_keepalive,
                          std::vector<char> &&data);

    enum force_yield_t {FORCE_YIELD, MAYBE_YIELD};
    void mailbox_read_coroutine(connectivity_cluster_t::connection_t *connection,
                                auto_drainer_t::lock_t connection_keepalive,
                                threadnum_t dest_thread,
                                raw_mailbox_t::id_t dest_mailbox_id,
                                std::vector<char> *stream_data,
                                int64_t stream_data_offset,
                                force_yield_t force_yield);
};

/* Note: disconnect_watcher_t keeps the connection alive for as long as it
exists, blocking reconnects from getting through. Avoid keeping the 
disconnect_watcher_t around for long after it gets pulsed. */
class disconnect_watcher_t : public signal_t, private signal_t::subscription_t {
public:
    disconnect_watcher_t(mailbox_manager_t *mailbox_manager, peer_id_t peer);
private:
    void run();
    auto_drainer_t::lock_t connection_keepalive;
};

#endif /* RPC_MAILBOX_MAILBOX_HPP_ */
