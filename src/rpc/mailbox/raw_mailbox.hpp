#ifndef RPC_MAILBOX_RAW_MAILBOX_HPP_
#define RPC_MAILBOX_RAW_MAILBOX_HPP_

#include "backtrace.hpp"
#include "concurrency/auto_drainer.hpp"
#include "rpc/connectivity/peer_id.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "threading.hpp"

class mailbox_manager_t;
class mailbox_read_callback_t;
class mailbox_write_callback_t;

class raw_mailbox_t : public home_thread_mixin_t {
public:
    struct address_t;
    typedef uint64_t id_t;

#ifndef NDEBUG
    lazy_backtrace_formatter_t bt;
#endif

private:
    friend class mailbox_manager_t;
    friend class raw_mailbox_writer_t;
    friend void send_write(mailbox_manager_t *, address_t, mailbox_write_callback_t *);

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
        friend void send_write(mailbox_manager_t *, raw_mailbox_t::address_t, mailbox_write_callback_t *callback);
        friend class raw_mailbox_t;
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

#endif  // RPC_MAILBOX_RAW_MAILBOX_HPP_
