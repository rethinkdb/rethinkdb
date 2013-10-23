#include <limits>

#include "rdb_protocol/explain.hpp"
#include "rdb_protocol/datum.hpp"

namespace explain {

RDB_IMPL_ME_SERIALIZABLE_2(sample_info_t, mean_duration_, n_samples_);

sample_info_t::sample_info_t()
    : mean_duration_(0), n_samples_(0) { }

sample_info_t::sample_info_t(ticks_t mean_duration, size_t n_samples)
    : mean_duration_(mean_duration), n_samples_(n_samples) { }

RDB_IMPL_ME_SERIALIZABLE_5(event_t, type_, description_,
        n_parallel_jobs_, when_, sample_info_);

event_t::event_t()
    : type_(STOP), when_(get_ticks()) { }

event_t::event_t(event_t::type_t type)
    : type_(type), when_(get_ticks()) { }

event_t::event_t(const std::string &description)
    : type_(START), description_(description),
      when_(get_ticks()) { }

event_t::event_t(const sample_info_t sample_info)
    : type_(SAMPLE), sample_info_(sample_info) { }

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

counted_t<const ql::datum_t> construct_sample(
        const sample_info_t &sample_info) {
    std::map<std::string, counted_t<const ql::datum_t> > res;
    guarantee(sample_info.mean_duration_ < std::numeric_limits<double>::max());
    guarantee(sample_info.n_samples_< std::numeric_limits<double>::max());
    double mean_duration = static_cast<double>(sample_info.mean_duration_) / MILLION;
    double n_samples = static_cast<double>(sample_info.n_samples_);
    res["mean_duration(ms)"] = make_counted<const ql::datum_t>(mean_duration);
    res["n_samples"] = make_counted<const ql::datum_t>(n_samples);
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
            case event_t::SAMPLE: {
                event_t sample = **begin;
                (*begin)++;
                res.push_back(construct_sample(sample.sample_info_));
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
            case event_t::SAMPLE:
                debugf("Sample.");
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
    : parent_(parent), starter_(description, parent),
      total_time(0), n_samples(0)
{
    if (parent_) {
        parent_->start_sample(&event_log_);
    }
}

sampler_t::sampler_t(const std::string &description, const scoped_ptr_t<trace_t> &parent)
    : sampler_t(description, parent.get_or_null()) { }

ticks_t duration(const event_log_t &event_log) {
    guarantee(!event_log.empty());
    if (event_log.at(0).type_ == event_t::START) {
        guarantee(event_log.back().type_ == event_t::STOP);
        return event_log.back().when_ - event_log.at(0).when_;
    } else {
        guarantee(event_log.at(0).type_ == event_t::SPLIT);
        ticks_t start = event_log.at(0).when_, res = 0;
        size_t jobs_counter = event_log.at(0).n_parallel_jobs_;
        auto it = event_log.begin() + 1;

        size_t depth = 0;
        for (;it != event_log.end(); ++it) {
            guarantee(jobs_counter != 0);
            if (it->type_ == event_t::STOP) {
                if (depth == 0) {
                    res += (it->when_ - start) / event_log.at(0).n_parallel_jobs_;
                    jobs_counter--;
                } else {
                    depth--;
                }
            } else if (it->type_ == event_t::START) {
                depth++;
            } else if (it->type_ == event_t::SPLIT) {
                depth += it->n_parallel_jobs_;
            }
        }
        guarantee(jobs_counter == 0);
        return res;
    }
}

void sampler_t::new_sample() {
    if (!event_log_.empty()) {
        n_samples++;
        total_time += duration(event_log_);
    }

    event_log_.clear();
}

sampler_t::~sampler_t() {
    new_sample();
    if (parent_) {
        parent_->stop_sample(sample_info_t(total_time / n_samples, n_samples));
    }
}

trace_t::trace_t()
    : redirected_event_log_(NULL) { }

counted_t<const ql::datum_t> trace_t::as_datum() const {
    guarantee(!redirected_event_log_);
    event_log_t::const_iterator begin = event_log_.begin();
    return construct_datum(&begin, event_log_.end());
}

event_log_t &&trace_t::get_event_log() {
    guarantee(!redirected_event_log_);
    return std::move(event_log_);
}

void trace_t::start(const std::string &description) {
    event_log_target()->push_back(event_t(description));
}

void trace_t::stop() {
    event_log_target()->push_back(event_t());
}

void trace_t::start_split() {
    event_log_target()->push_back(event_t(event_t::SPLIT));
}

void trace_t::stop_split(size_t n_parallel_jobs_, const event_log_t &par_event_log) {
    guarantee(event_log_.back().type_ == event_t::SPLIT);
    event_log_target()->back().n_parallel_jobs_ = n_parallel_jobs_;
    event_log_target()->insert(event_log_.end(), par_event_log.begin(), par_event_log.end());
}

void trace_t::start_sample(event_log_t *event_log) {
    redirected_event_log_ = event_log;
}

void trace_t::stop_sample(const sample_info_t &sample_info) {
    redirected_event_log_ = NULL;
    event_log_target()->push_back(event_t(sample_info));
}

event_log_t *trace_t::event_log_target() {
    if (redirected_event_log_) {
        return redirected_event_log_;
    } else {
        return &event_log_;
    }
}

} //namespace explain
