#include <limits>

#include "rdb_protocol/explain.hpp"
#include "rdb_protocol/datum.hpp"

explain_t::explain_t(const std::string &description)
    : description_(description), start_time_(get_ticks()), next_task(NULL)
{ }

explain_t *explain_t::new_task(const std::string &description) {
    guarantee(next_task == NULL);
    guarantee(parallel_tasks.empty(), "If a task has had parallel tasks created"
            " then you need to call finish_parallel_tasks instead of new_task.");
    finish();
    next_task = new explain_t(description);
    return next_task;
}

explain_t *explain_t::new_parallel_task(const std::string &description) {
    finish();
    parallel_tasks.push_back(new explain_t(description));
    return parallel_tasks.back();
}

explain_t *explain_t::finish_parallel_tasks(
        const std::vector<explain_t *> &tasks,
        const std::string &description) {
    for (auto it = tasks.begin(); it != tasks.end(); ++it) {
        guarantee((*it)->next_task == NULL);
        (*it)->finish();
    }

    next_task = new explain_t(description);
    return next_task;
}

void explain_t::finish() {
    ticks_ = get_ticks() - start_time_;
}

counted_t<const ql::datum_t> explain_t::as_datum() const {
    std::vector<counted_t<const ql::datum_t> > res;
    as_datum_helper(&res);
    return make_counted<const ql::datum_t>(std::move(res));
}

void explain_t::as_datum_helper(
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

counted_t<const ql::datum_t> explain_t::get_atom() const {
    std::map<std::string, counted_t<const ql::datum_t> > res;
    bool clobbered = res["description"] =
        make_counted<const ql::datum_t>(std::move(std::string(description_)));
    guarantee(!clobbered);
    guarantee(ticks_, "Someone tried to convert to a datum without calling `finish`.\n");
    guarantee(*ticks_ < std::numeric_limits<double>::max());
    clobbered = res["time"] = make_counted<const ql::datum_t>(static_cast<double>(*ticks_));
    guarantee(!clobbered);
    return make_counted<const ql::datum_t>(std::move(res));
}
