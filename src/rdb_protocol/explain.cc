#include <limits>

#include "rdb_protocol/explain.hpp"
#include "rdb_protocol/datum.hpp"

namespace explain {

task_t::task_t()
    : state_(UNINITIALIZED), next_task_(NULL) { }

task_t::task_t(const std::string &description)
    : next_task_(NULL)
{
    init(description);
}

task_t::task_t(const task_t &other)
    : state_(other.state_), description_(other.description_),
      start_time_(other.start_time_), ticks_(other.ticks_)
{
    for (auto it = other.parallel_tasks_.begin();
            it != other.parallel_tasks_.end(); ++it) {
        parallel_tasks_.emplace_back(new task_t(**it));
    }

    if (other.next_task_.has()) {
        next_task_.init(new task_t(*other.next_task_));
    }
}

task_t &task_t::operator=(const task_t &other) {
    state_ = other.state_;
    description_ = other.description_;
    start_time_ = other.start_time_;
    ticks_ = other.ticks_;

    for (auto it = other.parallel_tasks_.begin();
            it != other.parallel_tasks_.end(); ++it) {
        parallel_tasks_.emplace_back(new task_t(**it));
    }

    if (other.next_task_.has()) {
        next_task_.init(new task_t(*other.next_task_));
    }

    return *this;
}

void task_t::init(const std::string &description) {
    state_ = RUNNING;
    description_ = description;
    start_time_ = get_ticks();
}

bool task_t::is_initted() {
    return state_ != UNINITIALIZED;
}

task_t *task_t::new_task() {
    guarantee(!next_task_.has());
    finish();
    next_task_.init(new task_t());
    return next_task_.get();
}

task_t *task_t::new_task(const std::string &description) {
    guarantee(!next_task_.has());
    finish();
    next_task_.init(new task_t(description));
    return next_task_.get();
}

task_t *task_t::new_parallel_task() {
    parallel_tasks_.emplace_back(new task_t());
    return parallel_tasks_.back().get();
}

task_t *task_t::new_parallel_task(const std::string &description) {
    parallel_tasks_.emplace_back(new task_t(description));
    return parallel_tasks_.back().get();
}

task_t *task_t::tail() {
    if (!next_task_.has()) {
        return this;
    } else {
        return next_task_->tail();
    }
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

    if (!parallel_tasks_.empty()) {
        std::vector<counted_t<const ql::datum_t> > datum_parallel_tasks;
        for (auto it = parallel_tasks_.begin(); it != parallel_tasks_.end(); ++it) {
            datum_parallel_tasks.push_back((*it)->as_datum());
        }
        parent->push_back(make_counted<const ql::datum_t>(std::move(datum_parallel_tasks)));
    }

    if (next_task_.has()) {
        next_task_->as_datum_helper(parent);
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
    msg << parallel_tasks_.size();

    for (auto it = parallel_tasks_.begin(); it != parallel_tasks_.end(); ++it) {
        msg << **it;
    }

    /* serialize the next task */
    if (next_task_.has()) {
        msg << has_next_bool_t::HAS_NEXT;
        msg << *next_task_;
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
    parallel_tasks_.resize(array_size);

    /* deserialize the parallel_tasks */
    for (auto it = parallel_tasks_.begin(); it != parallel_tasks_.end(); ++it) {
        it->init(new task_t());
        RETURN_IF_NOT_SUCCESS(deserialize(s, it->get()));
    }

    /* deserialize the next_task_ */
    has_next_bool_t has_next;
    RETURN_IF_NOT_SUCCESS(deserialize(s, &has_next));

    if (has_next == has_next_bool_t::HAS_NEXT) {
        next_task_.init(new task_t);
        RETURN_IF_NOT_SUCCESS(deserialize(s, next_task_.get()));
    }

    return ARCHIVE_SUCCESS;
}

trace_t::trace_t()
    : root_(NULL), head_(NULL)
{ }

trace_t::trace_t(task_t *root) {
    init(root);
}

void trace_t::init(task_t *root) {
    root_ = root;
    head_ = (root ? root->tail() : root);
}

void trace_t::checkin(const std::string &description) {
    if (!head_) {
        return;
    }

    if (head_->is_initted()) {
        head_ = head_->new_task(description);
    } else {
        head_->init(description);
    }
}

void trace_t::add_task(task_t &&task) {
    if (!head_) {
        return;
    }

    *head_->new_task() = task;
    head_ = head_->tail();
}

void trace_t::add_parallel_task(task_t &&task) {
    if (!head_) {
        return;
    }

    *head_->new_parallel_task() = task;
}

counted_t<const ql::datum_t> trace_t::as_datum() {
    return root_->as_datum();
}

const task_t *trace_t::get_task() {
    return root_;
}


} //namespace explain
