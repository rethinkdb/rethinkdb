#ifndef __ARCH_LINUX_DISK_ACCOUNTING_HPP__
#define __ARCH_LINUX_DISK_ACCOUNTING_HPP__

#include <boost/function.hpp>
#include "containers/intrusive_list.hpp"
#include "concurrency/queue/accounting.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/queue/translate.hpp"

template<class payload_t>
struct accounting_diskmgr_t {

    accounting_diskmgr_t() :
        producer(&caster),
        caster(&queue)
        { }

    struct account_t;

    struct action_t : public payload_t, public intrusive_list_node_t<action_t> {
        account_t *account;
    };

    /* Each account on the `accounting_diskmgr_t` has its own
    `unlimited_fifo_queue_t` associated with it. Operations for that account
    queue up on that queue while they wait for the `accounting_queue_t` on the
    `accounting_diskmgr_t` to draw from that account. */
    struct account_t {
        account_t(accounting_diskmgr_t *par, int pri) :
            parent(par),
            account(&par->queue, &queue, pri)
            { }
    private:
        friend class accounting_diskmgr_t;
        accounting_diskmgr_t *parent;
        unlimited_fifo_queue_t<action_t *, intrusive_list_t<action_t> > queue;
        typename accounting_queue_t<action_t *>::account_t account;
    };

    void submit(action_t *a) {
        a->account->queue.push(a);
    }
    boost::function<void (action_t *)> done_fun;

    passive_producer_t<payload_t *> * const producer;
    void done(payload_t *p) {
        done_fun(static_cast<action_t *>(p));
    }

private:
    accounting_queue_t<action_t *> queue;
    casting_passive_producer_t<action_t *, payload_t *> caster;
};

#endif /* __ARCH_LINUX_DISK_ACCOUNTING_HPP__ */
