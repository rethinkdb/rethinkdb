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
    void init(const std::string &description);
    bool is_initted();
    task_t *new_task(const std::string &description);
    task_t *new_parallel_task(const std::string &description);
    task_t *finish_parallel_tasks(
            const std::vector<task_t *> &tasks,
            const std::string &description);

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
    state_t state_;
    std::string description_;
    ticks_t start_time_, ticks_;
    std::vector<scoped_ptr_t<task_t> > parallel_tasks_;
    scoped_ptr_t<task_t> next_task_;
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        task_t::state_t, int8_t,
        task_t::UNINITIALIZED, task_t::FINISHED);

} //namespace explain
#endif
