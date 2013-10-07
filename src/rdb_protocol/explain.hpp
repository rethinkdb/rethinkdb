// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_EXPLAIN_HPP_
#define RDB_PROTOCOL_EXPLAIN_HPP_

#include <vector>

#include "containers/counted.hpp"
#include "rpc/serialize_macros.hpp"
#include "utils.hpp"
#include "containers/scoped.hpp"

namespace ql {
class datum_t;
} //namespace ql

namespace explain {

class task_t {
public:
    task_t();
    explicit task_t(const std::string &description);
    task_t(const task_t &other);
    task_t &operator=(const task_t &other);
    task_t(task_t &&other);
    task_t &operator=(task_t &&other);

    void init(const std::string &description);
    bool is_initted();
    task_t *new_task();
    task_t *new_task(const std::string &description);
    task_t *new_parallel_task();
    task_t *new_parallel_task(const std::string &description);
    task_t *tail();
    void finish();

    counted_t<const ql::datum_t> as_datum();

    RDB_DECLARE_ME_SERIALIZABLE;

private:
    void as_datum_helper(std::vector<counted_t<const ql::datum_t> > *parent);
    counted_t<const ql::datum_t> get_atom();

public:
    /* Just to make serialization easier. */
    enum state_t {UNINITIALIZED, RUNNING, FINISHED};
private:
    friend class trace_t;
    state_t state_;
    std::string description_;
    ticks_t start_time_, ticks_;
    std::vector<scoped_ptr_t<task_t> > parallel_tasks_;
    scoped_ptr_t<task_t> next_task_;
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        task_t::state_t, int8_t,
        task_t::UNINITIALIZED, task_t::FINISHED);

/* An trace_t wraps a task and makes it a bit easier to use. */
class trace_t {
public:
    trace_t();
    explicit trace_t(task_t *root);
    void init(task_t *root);
    void checkin(const std::string &description);
    void add_task(task_t &&task);
    void merge_task(task_t &&task);
    void add_parallel_task(task_t &&task);
    void merge_parallel_tasks_from(task_t &&task);
    counted_t<const ql::datum_t> as_datum();
    const task_t *get_task();

private:
    task_t *root_;
    task_t *head_;
};

} //namespace explain
#endif
