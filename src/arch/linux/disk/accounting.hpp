#ifndef __ARCH_LINUX_DISK_ACCOUNTING_HPP__
#define __ARCH_LINUX_DISK_ACCOUNTING_HPP__

#include <boost/function.hpp>
#include "containers/intrusive_list.hpp"
#include <set>

template<class payload_t>
struct accounting_diskmgr_t {

    accounting_diskmgr_t(int target_queue_depth);
    ~accounting_diskmgr_t();

    struct account_t;

    struct action_t : public payload_t, public intrusive_list_node_t<action_t> {
        account_t *account;
    };

    struct account_t : public intrusive_list_node_t<account_t> {
        account_t(accounting_diskmgr_t *par, int pri) : parent(par), priority(pri) {
            rassert(priority > 0);
        }
    private:
        friend class accounting_diskmgr_t;
        accounting_diskmgr_t *parent;
        int priority;
        intrusive_list_t<action_t> queue;
    };

    void submit(action_t *a);
    boost::function<void (action_t *)> done_fun;

    boost::function<void (payload_t *)> submit_fun;
    void done(payload_t *);

private:
    intrusive_list_t<account_t> active_accounts;
    int total_priority;   // Sum of priorities of all active accounts
    void activate_account(account_t *a);
    void deactivate_account(account_t *a);

    /* Used to evenly remove from all active accounts */
    int last_dequeue_pointer;

    /* `num_active` is how many operations we have sent through submit_fun() and not yet heard
    back about. `target_num_active` is what we try to keep `num_active` at. */
    int target_num_active, num_active;
    void pump();
};

#include "arch/linux/disk/accounting.tcc"

#endif /* __ARCH_LINUX_DISK_ACCOUNTING_HPP__ */
