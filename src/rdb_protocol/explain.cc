#include <limits>

#include "rdb_protocol/explain.hpp"
#include "rdb_protocol/datum.hpp"

task_t::task_t(const std::string &description)
    : description_(description), start_time_(get_ticks()), next_task(NULL)
{ }

task_t::~task_t() {
    for (auto it = parallel_tasks.begin(); it != parallel_tasks.end(); ++it) {
        delete *it;
    }
    delete next_task;
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
    ticks_ = get_ticks() - start_time_;
}

counted_t<const ql::datum_t> task_t::as_datum() const {
    std::vector<counted_t<const ql::datum_t> > res;
    as_datum_helper(&res);
    return make_counted<const ql::datum_t>(std::move(res));
}

void task_t::as_datum_helper(
        std::vector<counted_t<const ql::datum_t> > *parent) const {
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

counted_t<const ql::datum_t> task_t::get_atom() const {
    std::map<std::string, counted_t<const ql::datum_t> > res;
    res["description"] =
        make_counted<const ql::datum_t>(std::move(std::string(description_)));
    guarantee(ticks_, "Someone tried to convert to a datum without calling `finish`.\n");
    guarantee(*ticks_ < std::numeric_limits<double>::max());
    res["time"] = make_counted<const ql::datum_t>(static_cast<double>(*ticks_));
    return make_counted<const ql::datum_t>(std::move(res));
}
