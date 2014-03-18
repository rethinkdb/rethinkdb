#include "arch/io/disk/accounting.hpp"

#include "containers/printf_buffer.hpp"

/* Each account on the `accounting_diskmgr_t` has its own
   `unlimited_fifo_queue_t` associated with it. Operations for that account
   queue up on that queue while they wait for the `accounting_queue_t` on the
   `accounting_diskmgr_t` to draw from that account. */
struct accounting_diskmgr_eager_account_t : public semaphore_available_callback_t {
    typedef accounting_diskmgr_action_t action_t;

    accounting_diskmgr_eager_account_t(accounting_diskmgr_t *par,
                                       int pri,
                                       int outstanding_requests_limit) :
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
    co_semaphore_t *get_outstanding_requests_limiter() {
        return &outstanding_requests_limiter;
    }

private:
    // It would be nice if we could just use a limited_fifo_queue to
    // implement the limitation of outstanding requests.
    // However this part of the code must not rely on coroutines, therefore
    // we have to implement that functionality manually.
    // throttled_queue contains requests which can not be put on queue right now,
    // because the number of outstanding requests has been exceeded
    intrusive_list_t<action_t> throttled_queue;
    unlimited_fifo_queue_t<action_t *, intrusive_list_t<action_t> > queue;
    static_semaphore_t outstanding_requests_limiter;
    accounting_queue_t<action_t *>::account_t account;
    auto_drainer_t::lock_t accounter_lock;

    DISABLE_COPYING(accounting_diskmgr_eager_account_t);
};

accounting_diskmgr_account_t::accounting_diskmgr_account_t(accounting_diskmgr_t *_par,
                                                           int _pri,
                                                           int _outstanding_requests_limit)
        : par(_par), pri(_pri),
          outstanding_requests_limit(_outstanding_requests_limit) { }

accounting_diskmgr_account_t::~accounting_diskmgr_account_t() {
    par->assert_thread();
}

void accounting_diskmgr_account_t::push(action_t *action) {
    maybe_init();
    if (!requests_drainer.has()) {
        // Create the requests_drainer now rather than during construction of the
        // accounting_diskmgr_account_t. We are doing that since
        // accounting_diskmgr_account_t can be constructed on any thread, and
        // auto_drainer_t doesn't like that.
        requests_drainer.init(new auto_drainer_t());
    }
    action->account_acq = requests_drainer->lock();
    eager_account->push(action);
}

void accounting_diskmgr_account_t::on_semaphore_available() {
    maybe_init();
    eager_account->on_semaphore_available();
}

co_semaphore_t *accounting_diskmgr_account_t::get_outstanding_requests_limiter() {
    maybe_init();
    return eager_account->get_outstanding_requests_limiter();
}

void accounting_diskmgr_account_t::maybe_init(){
    if (!eager_account.has()) {
        par->assert_thread();
        eager_account.init(new eager_account_t(par, pri, outstanding_requests_limit));
    }
}





void debug_print(printf_buffer_t *buf,
                 const accounting_diskmgr_action_t &action) {
    buf->appendf("accounting_diskmgr_action{...}<");
    const accounting_payload_t &parent_action = action;
    debug_print(buf, parent_action);
}


accounting_diskmgr_t::~accounting_diskmgr_t() {
    auto_drainer.reset();  // Make absolutely sure this happens first.
}

void accounting_diskmgr_t::submit(action_t *a) {
    a->account->push(a);
}

void accounting_diskmgr_t::done(accounting_payload_t *p) {
    // p really is an action_t...
    action_t *a = static_cast<action_t *>(p);
    a->account->get_outstanding_requests_limiter()->unlock(1);
    a->account_acq.reset();
    done_fun(static_cast<action_t *>(p));
}

