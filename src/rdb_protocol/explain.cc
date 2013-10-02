#include <limits>

#include "rdb_protocol/explain.hpp"
#include "rdb_protocol/datum.hpp"

namespace explain {

task_t::task_t()
    : state_(UNINITIALIZED), next_task(NULL) { }

task_t::task_t(const std::string &description)
    : next_task(NULL)
{
    init(description);
}

task_t::~task_t() {
    for (auto it = parallel_tasks.begin(); it != parallel_tasks.end(); ++it) {
        delete *it;
    }
    delete next_task;
}

void task_t::init(const std::string &description) {
    state_ = RUNNING;
    description_ = description;
    start_time_ = get_ticks();
}

bool task_t::is_initted() {
    return state_ != UNINITIALIZED;
}

task_t *task_t::new_task(const std::string &description) {
    guarantee(next_task == NULL);
    guarantee(parallel_tasks.empty(), "If a task has had parallel tasks created"
            " then you need to call finish_parallel_tasks instead of new_task.");
    finish();
    next_task = new task_t(description);
    return next_task;
}

task_t *task_t::new_parallel_task(const std::string &description) {
    finish();
    parallel_tasks.push_back(new task_t(description));
    return parallel_tasks.back();
}

task_t *task_t::finish_parallel_tasks(
        const std::vector<task_t *> &tasks,
        const std::string &description) {
    for (auto it = tasks.begin(); it != tasks.end(); ++it) {
        guarantee((*it)->next_task == NULL);
        (*it)->finish();
    }

    next_task = new task_t(description);
    return next_task;
}

void task_t::finish() {
    guarantee(state_ == RUNNING,
            "Can't call finish without calling init (or constructing with a description)");
    state_ = FINISHED;
    ticks_ = get_ticks() - start_time_;
}

counted_t<const ql::datum_t> task_t::as_datum() {
    std::vector<counted_t<const ql::datum_t> > res;
    as_datum_helper(&res);
    return make_counted<const ql::datum_t>(std::move(res));
}

void task_t::as_datum_helper(
        std::vector<counted_t<const ql::datum_t> > *parent) {
    parent->push_back(get_atom());

    if (!parallel_tasks.empty()) {
        std::vector<counted_t<const ql::datum_t> > datum_parallel_tasks;
        for (auto it = parallel_tasks.begin(); it != parallel_tasks.end(); ++it) {
            datum_parallel_tasks.push_back((*it)->as_datum());
        }
        parent->push_back(make_counted<const ql::datum_t>(std::move(datum_parallel_tasks)));
    }

    if (next_task != NULL) {
        next_task->as_datum_helper(parent);
    }
}

counted_t<const ql::datum_t> task_t::get_atom() {
    if (state_ != FINISHED) {
        finish();
    }
    std::map<std::string, counted_t<const ql::datum_t> > res;
    res["description"] =
        make_counted<const ql::datum_t>(std::move(std::string(description_)));
    guarantee(ticks_ < std::numeric_limits<double>::max());
    res["time"] = make_counted<const ql::datum_t>(static_cast<double>(ticks_));
    return make_counted<const ql::datum_t>(std::move(res));
}

enum class has_next_bool_t { HAS_NEXT, NO_NEXT };

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        has_next_bool_t, int8_t,
        has_next_bool_t::HAS_NEXT, has_next_bool_t::NO_NEXT);

void task_t::rdb_serialize(write_message_t &msg) const {
    /* serialize the simple fields. */
    msg << state_ << description_ << start_time_ << ticks_;

    /* serialize the parallel tasks if the exist */
    msg << parallel_tasks.size();

    for (auto it = parallel_tasks.begin(); it != parallel_tasks.end(); ++it) {
        msg << **it;
    }

    /* serialize the next task */
    if (next_task) {
        msg << has_next_bool_t::HAS_NEXT;
        msg << *next_task;
    } else {
        msg << has_next_bool_t::NO_NEXT;
    }
}

#define RETURN_IF_NOT_SUCCESS(val) \
{ \
    archive_result_t res = (val); \
    if (res != ARCHIVE_SUCCESS) { \
        return res; \
    } \
}

MUST_USE archive_result_t task_t::rdb_deserialize(read_stream_t *s) {
    /* deserialize the simple fields */
    RETURN_IF_NOT_SUCCESS(deserialize(s, &state_));
    RETURN_IF_NOT_SUCCESS(deserialize(s, &description_));
    RETURN_IF_NOT_SUCCESS(deserialize(s, &start_time_));
    RETURN_IF_NOT_SUCCESS(deserialize(s, &ticks_));
    size_t array_size;
    RETURN_IF_NOT_SUCCESS(deserialize(s, &array_size));
    parallel_tasks.resize(array_size, NULL);

    /* deserialize the parallel_tasks */
    for (auto it = parallel_tasks.begin(); it != parallel_tasks.end(); ++it) {
        *it = new task_t();
        RETURN_IF_NOT_SUCCESS(deserialize(s, *it));
    }

    /* deserialize the next_task */
    has_next_bool_t has_next;
    RETURN_IF_NOT_SUCCESS(deserialize(s, &has_next));

    if (has_next == has_next_bool_t::HAS_NEXT) {
        next_task = new task_t;
        RETURN_IF_NOT_SUCCESS(deserialize(s, next_task));
    } else {
        next_task = NULL;
    }

    return ARCHIVE_SUCCESS;
}

} //namespace explain
