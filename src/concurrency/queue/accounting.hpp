#ifndef __CONCURRENCY_QUEUE_ACCOUNTING_HPP__
#define __CONCURRENCY_QUEUE_ACCOUNTING_HPP__

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
struct accounting_queue_t :
    public passive_producer_t<value_t>,
    public home_thread_mixin_t
{
    accounting_queue_t() :
        passive_producer_t<value_t>(&available_var),
        total_shares(0),
        selector(0) { }

    ~accounting_queue_t() {
        rassert(active_accounts.empty());
        rassert(inactive_accounts.empty());
    }

    struct account_t :
        public intrusive_list_node_t<account_t>,
        private watchable_t<bool>::watcher_t
    {
        account_t(accounting_queue_t *p, passive_producer_t<value_t> *s, int shares) :
            parent(p), source(s), shares(shares), active(false)
        {
            on_thread_t thread_switcher(parent->home_thread);
            rassert(shares > 0);
            if (source->available->get()) activate();
            else parent->inactive_accounts.push_back(this);
            source->available->add_watcher(this);
            parent->available_var.set(!parent->active_accounts.empty());
        }
        ~account_t() {
            on_thread_t thread_switcher(parent->home_thread);
            parent->assert_thread();
            source->available->remove_watcher(this);
            if (active) deactivate();
            else parent->inactive_accounts.remove(this);
            parent->available_var.set(!parent->active_accounts.empty());
        }

    private:
        friend class accounting_queue_t;

        void on_watchable_changed() {
            parent->assert_thread();
            if (source->available->get() && !active) {
                parent->inactive_accounts.remove(this);
                activate();
            } else if (!source->available->get() && active) {
                deactivate();
                parent->inactive_accounts.push_back(this);
            }
            parent->available_var.set(!parent->active_accounts.empty());
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

    int total_shares, selector;

    watchable_var_t<bool> available_var;
    value_t produce_next_value() {
        assert_thread();
        selector %= total_shares;
        typename intrusive_list_t<account_t>::iterator it = active_accounts.begin();
        int count = 0;
        while (true) {
            count += (*it).shares;
            if (count > selector) break;
            it++;
        }
        selector++;
        return (*it).source->pop();
    }
};

#endif /* __CONCURRENCY_QUEUE_ACCOUNTING_HPP__ */
