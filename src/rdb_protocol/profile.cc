#include "rdb_protocol/profile.hpp"

#include <inttypes.h>

#include <limits>

#include "errors.hpp"
#include <boost/variant/static_visitor.hpp>

#include "containers/archive/stl_types.hpp"
#include "logger.hpp"
#include "rdb_protocol/math_utils.hpp"
#include "rdb_protocol/datum.hpp"

namespace profile {

start_t::start_t() { }

start_t::start_t(const std::string &description)
    : description_(description), when_(get_ticks()) { }

RDB_IMPL_ME_SERIALIZABLE_2(start_t, 0, description_, when_);

split_t::split_t() { }

split_t::split_t(size_t n_parallel_jobs)
    : n_parallel_jobs_(n_parallel_jobs) { }

RDB_IMPL_ME_SERIALIZABLE_1(split_t, 0, n_parallel_jobs_);

sample_t::sample_t() { }

sample_t::sample_t(const std::string &description,
        ticks_t mean_duration, size_t n_samples)
    : description_(description), mean_duration_(mean_duration),
      n_samples_(n_samples)
{ }

RDB_IMPL_ME_SERIALIZABLE_3(sample_t, 0, description_, mean_duration_, n_samples_);

stop_t::stop_t()
    : when_(get_ticks()) { }

RDB_IMPL_ME_SERIALIZABLE_1(stop_t, 0, when_);

counted_t<const ql::datum_t> construct_start(
        ticks_t duration, std::string &&description,
        counted_t<const ql::datum_t> sub_tasks) {
    std::map<std::string, counted_t<const ql::datum_t> > res;
    res["duration(ms)"] = make_counted<const ql::datum_t>(safe_to_double(duration) / MILLION);
    res["description"] = make_counted<const ql::datum_t>(std::move(description));
    res["sub_tasks"] = sub_tasks;
    return make_counted<const ql::datum_t>(std::move(res));
}

counted_t<const ql::datum_t> construct_split(
        counted_t<const ql::datum_t> par_tasks) {
    std::map<std::string, counted_t<const ql::datum_t> > res;
    res["parallel_tasks"] = par_tasks;
    return make_counted<const ql::datum_t>(std::move(res));
}

counted_t<const ql::datum_t> construct_sample(
        sample_t *sample) {
    std::map<std::string, counted_t<const ql::datum_t> > res;
    double mean_duration = safe_to_double(sample->mean_duration_) / MILLION;
    double n_samples = safe_to_double(sample->n_samples_);
    res["mean_duration(ms)"] = make_counted<const ql::datum_t>(mean_duration);
    res["n_samples"] = make_counted<const ql::datum_t>(n_samples);
    res["description"] = make_counted<const ql::datum_t>(std::move(sample->description_));
    return make_counted<const ql::datum_t>(std::move(res));
}

counted_t<const ql::datum_t> construct_datum(
        event_log_t::iterator *begin,
        event_log_t::iterator end);

class construct_datum_visitor_t : public boost::static_visitor<void> {
public:
    construct_datum_visitor_t(
        event_log_t::iterator *begin, event_log_t::iterator end,
        std::vector<counted_t<const ql::datum_t> > *res)
        : begin_(begin), end_(end), res_(res) { }

    void operator()(start_t &start) const {  // NOLINT(runtime/references)
        (*begin_)++;
        counted_t<const ql::datum_t> sub_tasks = construct_datum(begin_, end_);
        auto stop = boost::get<stop_t>(&**begin_);
        guarantee(stop);
        res_->push_back(construct_start(
            stop->when_ - start.when_, std::move(start.description_), sub_tasks));
        (*begin_)++;
    }
    void operator()(const split_t &split) const {
        (*begin_)++;
        std::vector<counted_t<const ql::datum_t> > parallel_tasks;
        for (size_t i = 0; i < split.n_parallel_jobs_; ++i) {
            parallel_tasks.push_back(construct_datum(begin_, end_));
            guarantee(boost::get<stop_t>(&**begin_));
            (*begin_)++;
        }
        res_->push_back(construct_split(
            make_counted<const ql::datum_t>(std::move(parallel_tasks))));
    }
    void operator()(sample_t &sample) const {  // NOLINT(runtime/references)
        (*begin_)++;
        res_->push_back(construct_sample(&sample));
    }
    void operator()(const stop_t &) const {
        //Nothing to do here
    }

private:
    event_log_t::iterator *begin_;
    event_log_t::iterator end_;
    std::vector<counted_t<const ql::datum_t> > *res_;
};

counted_t<const ql::datum_t> construct_datum(
        event_log_t::iterator *begin,
        event_log_t::iterator end) {
    std::vector<counted_t<const ql::datum_t> > res;

    construct_datum_visitor_t visitor(begin, end, &res);
    while (*begin != end && !boost::get<stop_t>(&**begin)) {
        boost::apply_visitor(visitor, **begin);
    }

    return make_counted<const ql::datum_t>(std::move(res));
}

class print_event_log_visitor_t : public boost::static_visitor<void> {
public:
    void operator()(const start_t &start) const {
        logINF("Start: %s.\n", start.description_.c_str());
    }
    void operator()(const split_t &split) const {
        logINF("Split: %zu.\n", split.n_parallel_jobs_);
    }
    void operator()(const sample_t &) const {
        logINF("Sample.");
    }
    void operator()(const stop_t &) const {
        logINF("Stop.\n");
    }
};

void print_event_log(const event_log_t &event_log) {
    print_event_log_visitor_t visitor;
    for (auto it = event_log.begin(); it != event_log.end(); ++it) {
        boost::apply_visitor(visitor, *it);
    }
}

starter_t::starter_t(const std::string &description, trace_t *parent) {
    init(description, parent);
}

starter_t::starter_t(const std::string &description, const scoped_ptr_t<trace_t> &parent) {
    init(description, parent.get_or_null());
}

starter_t::~starter_t() {
    if (parent_) {
        parent_->stop();
    }
}

void starter_t::init(const std::string &description, trace_t *parent) {
    parent_ = parent;
    if (parent_) {
        parent_->start(description);
    }
}

splitter_t::splitter_t(trace_t *parent) {
    init(parent);
}

splitter_t::splitter_t(const scoped_ptr_t<trace_t> &parent) {
    init(parent.get_or_null());
}

void splitter_t::give_splits(
    size_t n_parallel_jobs, const event_log_t &event_log) {
    n_parallel_jobs_ = n_parallel_jobs;
    event_log_ = event_log;
}

splitter_t::~splitter_t() {
    if (parent_) {
        parent_->stop_split(n_parallel_jobs_, event_log_);
    }
}

void splitter_t::init(trace_t *parent) {
    parent_ = parent;
    n_parallel_jobs_ = 0;

    if (parent_) {
        parent_->start_split();
    }
}

sampler_t::sampler_t(const std::string &description, trace_t *parent) {
    init(description, parent);
}

sampler_t::sampler_t(const std::string &description, const scoped_ptr_t<trace_t> &parent) {
    init(description, parent.get_or_null());
}

void sampler_t::init(const std::string &description, trace_t *parent) {
    parent_ = parent;
    description_ = description;
    total_time_ = 0;
    n_samples_ = 0;
    if (parent_) {
        parent_->start_sample(&event_log_);
    }
}

ticks_t duration(const event_log_t &event_log) {
    guarantee(!event_log.empty());
    if (auto start = boost::get<start_t>(&event_log.at(0))) {
        auto stop = boost::get<stop_t>(&event_log.back());
        guarantee(stop);
        return stop->when_ - start->when_;
    } else {
        //This is a code path that is currently never hit and that will go a
        //way when we implement more meaningful sampling functions.
        return 0;
    }
}

void sampler_t::new_sample() {
    if (!event_log_.empty()) {
        n_samples_++;
        total_time_ += duration(event_log_);
    }

    event_log_.clear();
}

sampler_t::~sampler_t() {
    new_sample();
    if (parent_) {
        if (n_samples_ > 0) {
            parent_->stop_sample(description_, total_time_ / n_samples_, n_samples_, &event_log_);
        } else {
            parent_->stop_sample(&event_log_);
        }
    }
}

disabler_t::disabler_t(trace_t *parent) {
    init(parent);
}

disabler_t::disabler_t(const scoped_ptr_t<trace_t> &parent) {
    init(parent.get_or_null());
}

disabler_t::~disabler_t() {
    if (parent_) {
        parent_->enable();
    }
}

void disabler_t::init(trace_t *parent) {
    parent_ = parent;
    if (parent_) {
        parent_->disable();
    }
}

trace_t::trace_t()
    : redirected_event_log_(NULL), disabled_ref_count(0) { }

counted_t<const ql::datum_t> trace_t::as_datum() {
    guarantee(!redirected_event_log_);
    event_log_t::iterator begin = event_log_.begin();
    return construct_datum(&begin, event_log_.end());
}

event_log_t trace_t::extract_event_log() RVALUE_THIS {
    // These guarantees imply that this trace_t gets left in a default-constructed
    // state (which is valid, thereby acceptable for an RVALUE_THIS function).
    guarantee(redirected_event_log_ == NULL);
    guarantee(disabled_ref_count == 0);
    return std::move(event_log_);
}

void trace_t::start(const std::string &description) {
    if (disabled()) { return; }
    //debugf("Start %s %p.\n", description.c_str(), this);
    event_log_target()->push_back(start_t(description));
}

void trace_t::stop() {
    if (disabled()) { return; }
    //debugf("Stop %p.\n", this);
    event_log_target()->push_back(stop_t());
}

void trace_t::start_split() {
    if (disabled()) { return; }
    //debugf("Start split %p.\n", this);
    event_log_target()->push_back(split_t());
}

void trace_t::stop_split(size_t n_parallel_jobs_, const event_log_t &par_event_log) {
    if (disabled()) { return; }
    //debugf("Stop split %zu, %p.\n", n_parallel_jobs_, this);
    auto split = boost::get<split_t>(&event_log_target()->back());
    guarantee(split);
    split->n_parallel_jobs_ = n_parallel_jobs_;
    event_log_target()->insert(event_log_target()->end(), par_event_log.begin(), par_event_log.end());
}

void trace_t::start_sample(event_log_t *event_log) {
    if (disabled()) { return; }
    //debugf("Start sample %p.\n", this);
    /* This is a tad hacky. We currently don't  allow samples within samples.
     * And if someone tries to do it the inner sample winds up just being a
     * no-op. We should see if in practice this is something we actually want
     * to support. */
    if (!redirected_event_log_) {
        redirected_event_log_ = event_log;
    }
}

void trace_t::stop_sample(const std::string &description,
        ticks_t mean_duration, size_t n_samples, event_log_t *event_log) {
    if (disabled()) { return; }
    //debugf("Stop sample %s, %p.\n", description.c_str(), this);
    /* Don't reset the redirected_event_log_ if the sampler_t wasn't
     * actually being redirected to. The predicate fails when the
     * innter samplers in nested sampler_ts are destructed. */
    if (event_log == redirected_event_log_) {
        redirected_event_log_ = NULL;
    }
    event_log_target()->push_back(sample_t(description, mean_duration, n_samples));
}

void trace_t::stop_sample(event_log_t *event_log) {
    if (disabled()) { return; }
    //debugf("Stop sample %p.\n", this);
    /* Don't reset the redirected_event_log_ if the sampler_t wasn't
     * actually being redirected to. The predicate fails when the
     * innter samplers in nested sampler_ts are destructed. */
    if (event_log == redirected_event_log_) {
        redirected_event_log_ = NULL;
    }
}

void trace_t::disable() {
    disabled_ref_count++;
}
void trace_t::enable() {
    disabled_ref_count--;
}

bool trace_t::disabled() {
    return disabled_ref_count > 0;
}

event_log_t *trace_t::event_log_target() {
    if (redirected_event_log_) {
        return redirected_event_log_;
    } else {
        return &event_log_;
    }
}

} //namespace profile 
