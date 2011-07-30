#ifndef __ARCH_IO_DISK_ACCOUNTING_HPP__
#define __ARCH_IO_DISK_ACCOUNTING_HPP__

#include <boost/function.hpp>
#include "containers/intrusive_list.hpp"
#include "concurrency/queue/accounting.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "arch/io/disk.hpp"

/* `casting_passive_producer_t` is useful when you have a
`passive_producer_t<X>` but you need a `passive_producer_t<Y>`, where `X` can
be cast to `Y`. */

template<class input_t, class output_t>
struct casting_passive_producer_t : public passive_producer_t<output_t> {

    casting_passive_producer_t(passive_producer_t<input_t> *source) :
        passive_producer_t<output_t>(source->available), source(source) { }

    output_t produce_next_value() {
        /* Implicit cast from `input_t` to `output_t` in return */
        return source->pop();
    }

private:
    passive_producer_t<input_t> *source;
};

/* `accounting_diskmgr_t` shares disk throughput proportionally between a
number of different "accounts". */

template<class payload_t>
struct accounting_diskmgr_t {

    accounting_diskmgr_t(int batch_factor) :
        producer(&caster),
        queue(batch_factor),
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
    struct account_t : public semaphore_available_callback_t {
        account_t(accounting_diskmgr_t *par, int pri, int outstanding_requests_limit) :
                parent(par),
                outstanding_requests_limiter(outstanding_requests_limit == UNLIMITED_OUTSTANDING_REQUESTS ? SEMAPHORE_NO_LIMIT : outstanding_requests_limit),
                account(&par->queue, &queue, pri) {
            rassert(outstanding_requests_limit == UNLIMITED_OUTSTANDING_REQUESTS || outstanding_requests_limit > 0);
        }
    private:
        friend class accounting_diskmgr_t;
        accounting_diskmgr_t *parent;

        // It would be nice if we could just use a limited_fifo_queue to
        // implement the limitation of outstanding requests.
        // However this part of the code must not rely on coroutines, therefore
        // we have to implement that functionality manually.
        // throttled_queue contains requests which can not be put on queue right now,
        // because the number of outstanding requests has been exceeded
        intrusive_list_t<action_t> throttled_queue;
        unlimited_fifo_queue_t<action_t *, intrusive_list_t<action_t> > queue;
        semaphore_t outstanding_requests_limiter;
        void push(action_t *action) {
            throttled_queue.push_back(action);
            outstanding_requests_limiter.lock(this, 1);
        }
        void on_semaphore_available() {
            action_t *action = throttled_queue.head();
            throttled_queue.pop_front();
            queue.push(action);
        }

        typename accounting_queue_t<action_t *>::account_t account;
    };

    void submit(action_t *a) {
        a->account->push(a);
    }
    boost::function<void (action_t *)> done_fun;

    passive_producer_t<payload_t *> * const producer;
    void done(payload_t *p) {
        // p really is an action_t...
        action_t *a = static_cast<action_t *>(p);
        a->account->outstanding_requests_limiter.unlock(1);
        done_fun(static_cast<action_t *>(p));
    }

private:
    accounting_queue_t<action_t *> queue;
    casting_passive_producer_t<action_t *, payload_t *> caster;
};

#endif /* __ARCH_IO_DISK_ACCOUNTING_HPP__ */
