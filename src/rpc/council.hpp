#ifndef __RPC_COUNCIL_HPP__
#define __RPC_COUNCIL_HPP__

#include "rpc/rpc.hpp"
#include "rpc/serialize/serialize_macros.hpp"
#include <boost/variant.hpp>
#include "concurrency/mutex.hpp"

/* council_t provides a mechanism to keep track of a synchronized value across
many machines. A "council" consists of a bunch of council_t instances,
possibly on separate machines. It is parameterized on a value_t, which must be
serializable. All of the council_ts have the same value_t at all times. Call
apply() on any council member to change the value. apply() takes a diff_t,
which must be serializable. Each council_t's updater function will be called
with the diff_t and a mutable pointer to the current value_t. New members can
be added to the council by calling the council_t constructor and passing a
council_t::address_t. Members can leave the council simply by destroying their
council_t objects. */

template<class value_t, class diff_t>
class council_t : public home_thread_mixin_t {

private:

    /* TODO: This doesn't correctly handle the case where a council member
    crashes. */

    /* The updater is the function we use to apply diffs. */
    typedef boost::function<void(const diff_t&, value_t*)> updater_t;

    struct inner_address_t;

    /* This is the current state of the council: a list of members and a current
    value. This is the struct that we send to new things that join. */
    struct state_t {
        /* TODO: std::vector<> of a forward-declared class might not work on some
        compilers. */
        std::vector<inner_address_t> peers;
        value_t value;
        int change_counter;   // For sanity checking
        state_t() { }
        state_t(const value_t &v, inner_address_t a1) : value(v), change_counter(0) {
            peers.push_back(a1);
        }
        RDB_MAKE_ME_SERIALIZABLE_3(peers, value, change_counter)
    };

    /* There are three types of changes that we can send to the council: A new member
    joining the council, a member leaving the council, and a change to the council's
    current value. */
    struct join_message_t {
        /* Use scoped_ptr only because inner_address_t is not a complete
        type at this point. */
        boost::scoped_ptr<inner_address_t> joiner;
        join_message_t() { }
        join_message_t(inner_address_t a) :
            joiner(new inner_address_t(a)) { }
        join_message_t(const join_message_t &m) :
            joiner(new inner_address_t(*m.joiner)) { }
        join_message_t &operator=(const join_message_t &m) {
            joiner.reset(new inner_address_t(*m.joiner));
            return *this;
        }
        RDB_MAKE_ME_SERIALIZABLE_1(joiner)
    };
    struct leave_message_t {
        /* "who" is an index in the "peers" array */
        int who;
        leave_message_t() { }
        leave_message_t(int w) : who(w) { }
        RDB_MAKE_ME_SERIALIZABLE_1(who)
    };
    typedef boost::variant<join_message_t, leave_message_t, diff_t> change_message_t;

    /* Each member of the council exposes two mailboxes to the others. The
    lock_mailbox_t is used for acquiring the council-wide lock. The change_mailbox_t
    is used for distributing changes after the lock is held. Distributing a change
    implicitly releases the lock. */
    typedef sync_mailbox_t < bool(int)                   > lock_mailbox_t;
    typedef async_mailbox_t< void(change_message_t, int) > change_mailbox_t;
    struct inner_address_t {
        typename lock_mailbox_t::address_t lock_address;
        typename change_mailbox_t::address_t change_address;
        inner_address_t() { }
        inner_address_t(council_t *c) :
            lock_address(&c->lock_mailbox),
            change_address(&c->change_mailbox)
            { }
        RDB_MAKE_ME_SERIALIZABLE_2(lock_address, change_address);
    };

    /* To join the council, send a message to the greeting mailbox of one of the
    council members. Give your inner_address_t and get in return the state_t of
    the council. */
    typedef sync_mailbox_t<state_t(inner_address_t)> greeting_mailbox_t;

    updater_t updater;

    // The startup cond prevents race conditions where we receive a lock message
    // before we've received a response to our greeting.
    multi_cond_t startup_cond;

    lock_mailbox_t lock_mailbox;
    change_mailbox_t change_mailbox;

    greeting_mailbox_t greeting_mailbox;

    /* The global lock is used to prevent multiple council members from making
    concurrent changes in such a way that there are race conditions. The variable
    "global_lock_holder" holds the ID of the highest-priority council member that
    currently wants the global lock. */
    static const int GLOBAL_LOCK_AVAILABLE = -1;
    int global_lock_holder;
    mutex_t global_lock;

    /* The local lock is used to resolve conflicts between multiple concurrent
    actions coming from this particular council member. */
    mutex_t local_lock;

    /* "state" is our copy of the council's state. "my_id" is our current ID
    number within the council. */
    state_t state;
    int my_id;

    void change(const change_message_t &msg);
    bool handle_lock(int locker);
    void handle_change(const change_message_t &msg, int counter);
    state_t handle_greeting(inner_address_t);

public:
    /* Start a new council with an initial value */
    council_t(updater_t, value_t);

    /* The address is an object that can be passed around to things that might
    want to join the council. */
    struct address_t {
    public:
        address_t();
        address_t(council_t *);
        RDB_MAKE_ME_SERIALIZABLE_1(inner_addr)
    private:
        friend class council_t;
        typename greeting_mailbox_t::address_t inner_addr;
    };

    /* Join a council */
    council_t(updater_t, address_t);

    /* Leave the council */
    ~council_t();

    /* Call to get the current value */
    const value_t &get_value();

    /* Call this to apply a diff to everything on the council */
    void apply(const diff_t &);
};

#include "rpc/council.tcc"

#endif /* __RPC_COUNCIL_HPP__ */

