#ifndef ARCH_IO_DISK_ACCOUNTING_HPP_
#define ARCH_IO_DISK_ACCOUNTING_HPP_

#include <boost/function.hpp>
#include "containers/intrusive_list.hpp"
#include "containers/scoped.hpp"
#include "concurrency/queue/accounting.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "arch/io/disk.hpp"

/* `casting_passive_producer_t` is useful when you have a
`passive_producer_t<X>` but you need a `passive_producer_t<Y>`, where `X` can
be cast to `Y`. */

template<class input_t, class output_t>
struct casting_passive_producer_t : public passive_producer_t<output_t> {
    explicit casting_passive_producer_t(passive_producer_t<input_t> *_source) :
        passive_producer_t<output_t>(_source->available), source(_source) { }

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
class accounting_diskmgr_t : home_thread_mixin_t {
public:

    explicit accounting_diskmgr_t(int batch_factor)
        : producer(&caster),
          queue(batch_factor),
          caster(&queue),
          auto_drainer(new auto_drainer_t()) { }
    ~accounting_diskmgr_t() {
        auto_drainer.reset(); //Make absolutely sure this happens first
    }

    struct account_t;

    struct action_t : public payload_t, public intrusive_list_node_t<action_t> {
        account_t *account;
    };

    /* Each account on the `accounting_diskmgr_t` has its own
    `unlimited_fifo_queue_t` associated with it. Operations for that account
    queue up on that queue while they wait for the `accounting_queue_t` on the
    `accounting_diskmgr_t` to draw from that account. */
    struct eager_account_t : public semaphore_available_callback_t {
        eager_account_t(accounting_diskmgr_t *par, int pri, int outstanding_requests_limit) :
                parent(par),
                outstanding_requests_limiter(outstanding_requests_limit == UNLIMITED_OUTSTANDING_REQUESTS ? SEMAPHORE_NO_LIMIT : outstanding_requests_limit),
                account(&par->queue, &queue, pri),
                accounter_lock(par->get_auto_drainer()) {
            rassert(outstanding_requests_limit == UNLIMITED_OUTSTANDING_REQUESTS || outstanding_requests_limit > 0);
        }

        void push(action_t *action) {
            throttled_queue.push_back(action);
            outstanding_requests_limiter.lock(this, 1);
        }
        void on_semaphore_available() {
            action_t *action = throttled_queue.head();
            throttled_queue.pop_front();
            queue.push(action);
        }
        semaphore_t &get_outstanding_requests_limiter() {
            return outstanding_requests_limiter;
        }

    private:
        friend struct account_t;
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
        typename accounting_queue_t<action_t *>::account_t account;
        auto_drainer_t::lock_t accounter_lock;
    };

    struct account_t {
        account_t(accounting_diskmgr_t *_par, int _pri, int _outstanding_requests_limit):
            par(_par), pri(_pri),
            outstanding_requests_limit(_outstanding_requests_limit) { }
        virtual ~account_t() { rassert(get_thread_id() == par->home_thread()); }

        void push(action_t *action) {
            maybe_init();
            eager_account->push(action);
        }
        void on_semaphore_available() {
            maybe_init();
            eager_account->on_semaphore_available();
        }
        semaphore_t &get_outstanding_requests_limiter() {
            maybe_init();
            return eager_account->get_outstanding_requests_limiter();
        }

    private:
        void maybe_init(){
            if (!eager_account.has()) {
                rassert(get_thread_id() == par->home_thread());
                eager_account.init(new eager_account_t(par, pri, outstanding_requests_limit));
            }
        }
        accounting_diskmgr_t *par;
        int pri;
        int outstanding_requests_limit;
        scoped_ptr_t<eager_account_t> eager_account;
    };

    void submit(action_t *a) {
        a->account->push(a);
    }
    boost::function<void (action_t *)> done_fun;

    passive_producer_t<payload_t *> * const producer;
    void done(payload_t *p) {
        // p really is an action_t...
        action_t *a = static_cast<action_t *>(p);
        a->account->get_outstanding_requests_limiter().unlock(1);
        done_fun(static_cast<action_t *>(p));
    }

    auto_drainer_t *get_auto_drainer() {
        return auto_drainer.get();
    }
private:
    accounting_queue_t<action_t *> queue;
    casting_passive_producer_t<action_t *, payload_t *> caster;
    scoped_ptr_t<auto_drainer_t> auto_drainer;
};

#endif /* ARCH_IO_DISK_ACCOUNTING_HPP_ */
