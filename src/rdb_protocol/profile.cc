#include <limits>

#include "rdb_protocol/profile.hpp"
#include "rdb_protocol/datum.hpp"

namespace profile {

start_t::start_t() { }

start_t::start_t(const std::string &description)
    : description_(description), when_(get_ticks()) { }

RDB_IMPL_ME_SERIALIZABLE_2(start_t, description_, when_);

split_t::split_t() { }

split_t::split_t(size_t n_parallel_jobs)
    : n_parallel_jobs_(n_parallel_jobs) { }

RDB_IMPL_ME_SERIALIZABLE_1(split_t, n_parallel_jobs_);

sample_t::sample_t() { }

sample_t::sample_t(const std::string &description,
        ticks_t mean_duration, size_t n_samples)
    : description_(description), mean_duration_(mean_duration),
      n_samples_(n_samples)
{ }

RDB_IMPL_ME_SERIALIZABLE_3(sample_t, description_, mean_duration_, n_samples_);

stop_t::stop_t()
    : when_(get_ticks()) { }

RDB_IMPL_ME_SERIALIZABLE_1(stop_t, when_);

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
        counted_t<const ql::datum_t> par_tasks) {
    std::map<std::string, counted_t<const ql::datum_t> > res;
    res["parallel_tasks"] = par_tasks;
    return make_counted<const ql::datum_t>(std::move(res));
}

counted_t<const ql::datum_t> construct_sample(
        sample_t *sample) {
    std::map<std::string, counted_t<const ql::datum_t> > res;
    guarantee(sample->mean_duration_ < std::numeric_limits<double>::max());
    guarantee(sample->n_samples_< std::numeric_limits<double>::max());
    double mean_duration = static_cast<double>(sample->mean_duration_) / MILLION;
    double n_samples = static_cast<double>(sample->n_samples_);
    res["mean_duration(ms)"] = make_counted<const ql::datum_t>(mean_duration);
    res["n_samples"] = make_counted<const ql::datum_t>(n_samples);
    res["description"] = make_counted<const ql::datum_t>(std::move(sample->description_));
    return make_counted<const ql::datum_t>(std::move(res));
}

counted_t<const ql::datum_t> construct_datum(
        event_log_t::iterator *begin,
        event_log_t::iterator end) {
    std::vector<counted_t<const ql::datum_t> > res;

    while (*begin != end && !boost::get<stop_t>(&**begin)) {
        if (auto start = boost::get<start_t>(&**begin)) {
            (*begin)++;
            counted_t<const ql::datum_t> sub_tasks = construct_datum(begin, end);
            auto stop = boost::get<stop_t>(&**begin);
            guarantee(stop);
            res.push_back(construct_start(
                stop->when_ - start->when_, std::move(start->description_), sub_tasks));
            (*begin)++;
        } else if (auto split = boost::get<split_t>(&**begin)) {
            (*begin)++;
            std::vector<counted_t<const ql::datum_t> > parallel_tasks;
            for (size_t i = 0; i < split->n_parallel_jobs_; ++i) {
                parallel_tasks.push_back(construct_datum(begin, end));
                guarantee(boost::get<stop_t>(&**begin));
                (*begin)++;
            }
            res.push_back(construct_split(
                make_counted<const ql::datum_t>(std::move(parallel_tasks))));
        } else if (auto sample = boost::get<sample_t>(&**begin)) {
            (*begin)++;
            res.push_back(construct_sample(sample));
        } else if (boost::get<stop_t>(&**begin)) {
            // Do nothing, we'll be breaking out of the loop
        } else {
            unreachable();
        }
    }

    return make_counted<const ql::datum_t>(std::move(res));
}

void print_event_log(const event_log_t &event_log) {
    for (auto it = event_log.begin(); it != event_log.end(); ++it) {
        if (auto start = boost::get<start_t>(&*it)) {
            debugf("Start: %s.\n", start->description_.c_str());
        } else if (auto split = boost::get<split_t>(&*it)) {
            debugf("Split: %zu.\n", split->n_parallel_jobs_);
        } else if (boost::get<sample_t>(&*it)) {
            debugf("Sample.");
        } else if (boost::get<stop_t>(&*it)) {
            debugf("Stop.\n");
        } else {
            unreachable();
        }

    }
}

starter_t::starter_t(const std::string &description, trace_t *parent) 
    : parent_(parent)
{
    if (parent_) {
        parent_->start(description);
    }
}

starter_t::starter_t(const std::string &description, const scoped_ptr_t<trace_t> &parent) 
    : starter_t(description, parent.get_or_null()) { }

starter_t::~starter_t() {
    if (parent_) {
        parent_->stop();
    }
}

splitter_t::splitter_t(trace_t *parent)
    : parent_(parent), received_splits_(false)
{
    if (parent_) {
        parent_->start_split();
    }
}

splitter_t::splitter_t(const scoped_ptr_t<trace_t> &parent)
    : splitter_t(parent.get_or_null()) { }

void splitter_t::give_splits(
    size_t n_parallel_jobs, const event_log_t &event_log) {
    n_parallel_jobs_ = n_parallel_jobs;
    event_log_ = event_log;
    received_splits_ = true;
}

splitter_t::~splitter_t() {
    if (parent_) {
        guarantee(received_splits_);
        parent_->stop_split(n_parallel_jobs_, event_log_);
    }
}

sampler_t::sampler_t(const std::string &description, trace_t *parent)
    : parent_(parent), description_(description),
      total_time_(0), n_samples_(0)
{
    if (parent_) {
        parent_->start_sample(&event_log_);
    }
}

sampler_t::sampler_t(const std::string &description, const scoped_ptr_t<trace_t> &parent)
    : sampler_t(description, parent.get_or_null()) { }

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
            parent_->stop_sample(description_, total_time_ / n_samples_, n_samples_);
        } else {
            parent_->stop_sample(description_, 0, 0);
        }
    }
}

trace_t::trace_t()
    : redirected_event_log_(NULL) { }

counted_t<const ql::datum_t> trace_t::as_datum() {
    guarantee(!redirected_event_log_);
    event_log_t::iterator begin = event_log_.begin();
    return construct_datum(&begin, event_log_.end());
}

event_log_t &&trace_t::get_event_log() RVALUE_THIS {
    guarantee(!redirected_event_log_);
    return std::move(event_log_);
}

void trace_t::start(const std::string &description) {
    event_log_target()->push_back(start_t(description));
}

void trace_t::stop() {
    event_log_target()->push_back(stop_t());
}

void trace_t::start_split() {
    event_log_target()->push_back(split_t());
}

void trace_t::stop_split(size_t n_parallel_jobs_, const event_log_t &par_event_log) {
    auto split = boost::get<split_t>(&event_log_target()->back());
    guarantee(split);
    split->n_parallel_jobs_ = n_parallel_jobs_;
    event_log_target()->insert(event_log_target()->end(), par_event_log.begin(), par_event_log.end());
}

void trace_t::start_sample(event_log_t *event_log) {
    redirected_event_log_ = event_log;
}

void trace_t::stop_sample(const std::string &description, 
        ticks_t mean_duration, size_t n_samples) {
    redirected_event_log_ = NULL;
    event_log_target()->push_back(sample_t(description, mean_duration, n_samples));
}

event_log_t *trace_t::event_log_target() {
    if (redirected_event_log_) {
        return redirected_event_log_;
    } else {
        return &event_log_;
    }
}

} //namespace profile 
