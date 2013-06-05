// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_DISK_ACCOUNTING_HPP_
#define ARCH_IO_DISK_ACCOUNTING_HPP_

#include "errors.hpp"
#include <boost/function.hpp>

#include "containers/intrusive_list.hpp"
#include "containers/scoped.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/queue/accounting.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/semaphore.hpp"
#include "arch/io/disk.hpp"
#include "arch/io/disk/stats.hpp"

/* `casting_passive_producer_t` is useful when you have a
`passive_producer_t<X>` but you need a `passive_producer_t<Y>`, where `X` can
be cast to `Y`. */

template <class input_t, class output_t>
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

typedef stats_diskmgr_2_t::action_t accounting_payload_t;

class accounting_diskmgr_t;

struct accounting_diskmgr_action_t;

struct accounting_diskmgr_eager_account_t;

struct accounting_diskmgr_account_t {
    typedef accounting_diskmgr_action_t action_t;

    accounting_diskmgr_account_t(accounting_diskmgr_t *_par,
                                 int _pri,
                                 int _outstanding_requests_limit);

    ~accounting_diskmgr_account_t();

    void push(action_t *action);
    void on_semaphore_available();
    semaphore_t *get_outstanding_requests_limiter();

private:
    typedef accounting_diskmgr_eager_account_t eager_account_t;

    void maybe_init();

    accounting_diskmgr_t *par;
    int pri;
    int outstanding_requests_limit;
    scoped_ptr_t<eager_account_t> eager_account;

    DISABLE_COPYING(accounting_diskmgr_account_t);
};

struct accounting_diskmgr_action_t
    : public intrusive_list_node_t<accounting_diskmgr_action_t>,
      public accounting_payload_t {
    accounting_diskmgr_account_t *account;
};

void debug_print(printf_buffer_t *buf,
                 const accounting_diskmgr_action_t &action);

class accounting_diskmgr_t : public home_thread_mixin_t {
public:
    explicit accounting_diskmgr_t(int batch_factor)
        : producer(&caster),
          queue(batch_factor),
          caster(&queue),
          auto_drainer(new auto_drainer_t()) { }

    ~accounting_diskmgr_t();

    typedef accounting_diskmgr_account_t account_t;

    typedef accounting_diskmgr_action_t action_t;

    void submit(action_t *a);

    boost::function<void (action_t *)> done_fun;

    passive_producer_t<accounting_payload_t *> * const producer;
    void done(accounting_payload_t *p);

    auto_drainer_t *get_auto_drainer() {
        return auto_drainer.get();
    }

private:
    friend struct accounting_diskmgr_eager_account_t;

    accounting_queue_t<action_t *> queue;
    casting_passive_producer_t<action_t *, accounting_payload_t *> caster;
    scoped_ptr_t<auto_drainer_t> auto_drainer;

    DISABLE_COPYING(accounting_diskmgr_t);
};

#endif /* ARCH_IO_DISK_ACCOUNTING_HPP_ */
