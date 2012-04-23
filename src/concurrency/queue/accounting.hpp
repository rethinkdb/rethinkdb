#ifndef CONCURRENCY_QUEUE_ACCOUNTING_HPP_
#define CONCURRENCY_QUEUE_ACCOUNTING_HPP_

#include "concurrency/queue/passive_producer.hpp"
#include "containers/intrusive_list.hpp"

/* `accounting_queue_t` is useful when you have some number of actors competing
for a shared resource, and you want them to be granted access to the resource in
a fixed ratio. The `accounting_queue_t` is a `passive_producer_t` that draws
from a number of other `passive_producer_t`s. To associate the
`accounting_queue_t` with a `passive_producer_t`, construct an
`accounting_queue_t::account_t` object. The `account_t` constructor takes a
`share` parameter; the relative ratio of the `share` parameters of the different
`account_t`s determines which `passive_producer_t`s the `accounting_queue_t`
will `pop()` from when its own `pop()` method is called. When one of the sub-
`passive_producer_t`s is not available, then it is ignored until it becomes
available. */

template<class value_t>
class accounting_queue_t :
    public passive_producer_t<value_t>,
    public home_thread_mixin_t
{
public:
    explicit accounting_queue_t(int _batch_factor) :
        passive_producer_t<value_t>(&available_control),
        total_shares(0),
        selector(0),
        batch_factor(_batch_factor) {

        rassert(batch_factor > 0);
    }

    ~accounting_queue_t() {
        rassert(active_accounts.empty());
        rassert(inactive_accounts.empty());
    }

    class account_t : private availability_callback_t, public intrusive_list_node_t<account_t> {
    public:
        account_t(accounting_queue_t *p, passive_producer_t<value_t> *s, int _shares)
            : parent(p), source(s), shares(_shares), active(false) {
            parent->assert_thread();
            rassert(shares > 0);
            if (source->available->get()) {
                activate();
            } else {
                parent->inactive_accounts.push_back(this);
            }
            source->available->set_callback(this);
            parent->available_control.set_available(!parent->active_accounts.empty());
        }
        ~account_t() {
            parent->assert_thread();
            source->available->unset_callback();
            if (active) {
                deactivate();
            } else {
                parent->inactive_accounts.remove(this);
            }
            parent->available_control.set_available(!parent->active_accounts.empty());
        }

    private:
        friend class accounting_queue_t;

        void on_source_availability_changed() {
            parent->assert_thread();
            if (source->available->get() && !active) {
                parent->inactive_accounts.remove(this);
                activate();
            } else if (!source->available->get() && active) {
                deactivate();
                parent->inactive_accounts.push_back(this);
            }
            parent->available_control.set_available(!parent->active_accounts.empty());
        }

        void activate() {
            active = true;
            parent->active_accounts.push_back(this);
            parent->total_shares += shares;
        }
        void deactivate() {
            active = false;
            parent->active_accounts.remove(this);
            parent->total_shares -= shares;
        }

        accounting_queue_t *parent;
        passive_producer_t<value_t> *source;
        int shares;
        bool active;
    };

private:
    friend class account_t;

    intrusive_list_t<account_t> active_accounts, inactive_accounts;

    int total_shares, selector, batch_factor;

    availability_control_t available_control;
    value_t produce_next_value() {
        assert_thread();

        selector %= total_shares * batch_factor;
        // TODO: Maybe that line should be like this instead?
        // It would be very fair, but there might be some issues with that (like
        // less sequential access patterns (<-- definitely a problem on rotational drives!),
        // maybe other problems)
        //selector = randint(total_shares);

        int batch_selector = selector / batch_factor;

        account_t *acct = active_accounts.head();
        int count = 0;
        for (;;) {
            count += acct->shares;
            if (count > batch_selector) {
                break;
            }
            acct = active_accounts.next(acct);
        }
        selector++;
        return acct->source->pop();
    }
};

#endif /* CONCURRENCY_QUEUE_ACCOUNTING_HPP_ */
