#include <limits>

#include "rdb_protocol/explain.hpp"
#include "rdb_protocol/datum.hpp"

namespace explain {

RDB_IMPL_ME_SERIALIZABLE_4(event_t, type_, description_, n_parallel_jobs_, when_);

event_t::event_t()
    : type_(STOP), when_(get_ticks()) { }

event_t::event_t(event_t::type_t type)
    : type_(type), when_(get_ticks()) { }

event_t::event_t(const std::string &description)
    : type_(START), description_(description),
      when_(get_ticks()) { }

counted_t<const ql::datum_t> construct_start(
        ticks_t duration, std::string &&description,
        counted_t<const ql::datum_t> sub_tasks) {
    std::map<std::string, counted_t<const ql::datum_t> > res;
    guarantee(duration < std::numeric_limits<double>::max());
    res["duration(ms)"] = make_counted<const ql::datum_t>(static_cast<double>(duration) / MILLION);
    res["description"] = make_counted<const ql::datum_t>(std::move(description));
    res["sub_tasks"] = sub_tasks;
    return make_counted<const ql::datum_t>(std::move(res));
}

counted_t<const ql::datum_t> construct_split(
        ticks_t duration, counted_t<const ql::datum_t> par_tasks) {
    std::map<std::string, counted_t<const ql::datum_t> > res;
    guarantee(duration < std::numeric_limits<double>::max());
    res["duration(ms)"] = make_counted<const ql::datum_t>(static_cast<double>(duration) / MILLION);
    res["parallel_tasks"] = par_tasks;
    return make_counted<const ql::datum_t>(std::move(res));
}

counted_t<const ql::datum_t> construct_datum(
        event_log_t::const_iterator *begin,
        event_log_t::const_iterator end) {
    std::vector<counted_t<const ql::datum_t> > res;

    while (*begin != end && (*begin)->type_ != event_t::STOP) {
        switch ((*begin)->type_) {
            case event_t::START: {
                event_t start = **begin;
                (*begin)++;
                counted_t<const ql::datum_t> sub_tasks = construct_datum(begin, end);
                if ((*begin)->type_ != event_t::STOP) {
                    sub_tasks = construct_datum(begin, end);
                }

                guarantee((*begin)->type_ == event_t::STOP);
                res.push_back(construct_start(
                    (*begin)->when_ - start.when_, std::move(start.description_), sub_tasks));
                (*begin)++;
            } break;
            case event_t::SPLIT: {
                event_t split = **begin;
                (*begin)++;
                std::vector<counted_t<const ql::datum_t> > parallel_tasks;
                for (size_t i = 0; i < split.n_parallel_jobs_; ++i) {
                    parallel_tasks.push_back(construct_datum(begin, end));
                    guarantee((*begin)->type_ == event_t::STOP);
                    (*begin)++;
                }
                res.push_back(construct_split(
                    (*begin)->when_ - split.when_,
                    make_counted<const ql::datum_t>(std::move(parallel_tasks))));
            } break;
            case event_t::STOP:
                break;
            default:
                unreachable();
        }
    }

    return make_counted<const ql::datum_t>(std::move(res));
}

void print_event_log(const event_log_t &event_log) {
    for (auto it = event_log.begin(); it != event_log.end(); ++it) {
        switch (it->type_) {
            case event_t::START:
                debugf("Start.\n");
                break;
            case event_t::SPLIT:
                debugf("Split: %zu.\n", it->n_parallel_jobs_);
                break;
            case event_t::STOP:
                debugf("Stop.\n");
                break;
            default:
                unreachable();
                break;
        }
    }
}


starter_t::starter_t(const std::string &description, trace_t *parent) 
    : parent_(parent)
{
    parent_->start(description);
}

starter_t::~starter_t() {
    parent_->stop();
}

splitter_t::splitter_t(trace_t *parent)
    : parent_(parent), received_splits_(false)
{
    parent_->start_split();
}

void splitter_t::give_splits(
    size_t n_parallel_jobs, const event_log_t &event_log) {
    n_parallel_jobs_ = n_parallel_jobs;
    event_log_ = event_log;
    received_splits_ = true;
}

splitter_t::~splitter_t() {
    guarantee(received_splits_);
    parent_->stop_split(n_parallel_jobs_, event_log_);
}

counted_t<const ql::datum_t> trace_t::as_datum() const {
    event_log_t::const_iterator begin = event_log_.begin();
    return construct_datum(&begin, event_log_.end());
}

event_log_t &&trace_t::get_event_log() {
    return std::move(event_log_);
}

void trace_t::start(const std::string &description) {
    event_log_.push_back(event_t(description));
}

void trace_t::stop() {
    event_log_.push_back(event_t());
}

void trace_t::start_split() {
    event_log_.push_back(event_t(event_t::SPLIT));
}

void trace_t::stop_split(size_t n_parallel_jobs_, const event_log_t &par_event_log) {
    guarantee(event_log_.back().type_ == event_t::SPLIT);
    event_log_.back().n_parallel_jobs_ = n_parallel_jobs_;
    event_log_.insert(event_log_.end(), par_event_log.begin(), par_event_log.end());
}

} //namespace explain
