/* Please avoid #include'ing this file from other headers unless you absolutely
 * need to. Please #include "perfmon/perfmon_types.hpp" instead. This helps avoid
 * potential circular dependency problems, since perfmons are used all over the
 * place but also depend on a significant chunk of our threading infrastructure
 * (by way of get_thread_id() & co).
 */
#ifndef PERFMON_HPP_
#define PERFMON_HPP_

#include <limits>
#include <string>
#include <map>

#include "config/args.hpp"
#include "containers/intrusive_list.hpp"
#include "perfmon/perfmon_types.hpp"
#include "concurrency/rwi_lock.hpp"
#include "utils.hpp"

#include "containers/archive/archive.hpp"

// Some arch/runtime declarations.
int get_num_threads();
int get_thread_id();

// Pad a value to the size of a cache line to avoid false sharing.
// TODO: This is implemented as a struct with subtraction rather than a union
// so that it gives an error when trying to pad a value bigger than
// CACHE_LINE_SIZE. If that's needed, this may have to be done differently.
// TODO: Use this in the rest of the perfmons, if it turns out to make any
// difference.

template<typename value_t>
struct cache_line_padded_t {
    value_t value;
    char padding[CACHE_LINE_SIZE - sizeof(value_t)];
};

/* The perfmon (short for "PERFormance MONitor") is responsible for gathering data about
various parts of the server. */

class perfmon_result_t {
    typedef std::map<std::string, perfmon_result_t *> internal_map_t;
public:
    enum perfmon_result_type_t {
        type_value,
        type_map,
    };

    perfmon_result_t();
    perfmon_result_t(const perfmon_result_t &);
    explicit perfmon_result_t(const std::string &);
    virtual ~perfmon_result_t();

    static perfmon_result_t make_string() {
        return perfmon_result_t(std::string());
    }

    static void alloc_string_result(perfmon_result_t **out) {
        *out = new perfmon_result_t(std::string());
    }

    static perfmon_result_t make_map() {
        return perfmon_result_t(internal_map_t());
    }

    static void alloc_map_result(perfmon_result_t **out) {
        *out = new perfmon_result_t(internal_map_t());
    }

    std::string *get_string() {
        rassert(type == type_value);
        return &value_;
    }

    const std::string *get_string() const {
        rassert(type == type_value);
        return &value_;
    }

    internal_map_t *get_map() {
        rassert(type == type_map);
        return &map_;
    }

    const internal_map_t *get_map() const {
        rassert(type == type_map);
        return &map_;
    }

    bool is_string() const {
        return type == type_value;
    }

    bool is_map() const {
        return type == type_map;
    }

    perfmon_result_type_t get_type() const {
        return type;
    }

    std::pair<internal_map_t::iterator, bool> insert(const std::string &name, perfmon_result_t *val) {
        std::string s(name);
        internal_map_t *map = get_map();
        rassert(map->count(name) == 0);
        return map->insert(std::pair<std::string, perfmon_result_t *>(s, val));
    }

    typedef internal_map_t::iterator iterator;
    typedef internal_map_t::const_iterator const_iterator;

    iterator begin() {
        return map_.begin();
    }

    iterator end() {
        return map_.end();
    }

    const_iterator begin() const {
        return map_.begin();
    }

    const_iterator end() const {
        return map_.end();
    }

private:
    explicit perfmon_result_t(const internal_map_t &);

    // We need these two friends for serialization, but we don't want to include the
    // serialization headers, neither we want to define the serializers here.
    friend write_message_t &operator<<(write_message_t &msg, const perfmon_result_t &thing);
    friend archive_result_t deserialize(read_stream_t *s, perfmon_result_t *thing);

    perfmon_result_type_t type;

    std::string value_;
    internal_map_t map_;

    void operator=(const perfmon_result_t &);
};

/* perfmon_get_stats() collects all the stats about the server and puts them
into the given perfmon_result_t object. It must be run in a coroutine and it blocks
until it is done. */

void perfmon_get_stats(perfmon_result_t *dest);

/* A perfmon_t represents a stat about the server.

To monitor something, declare a global variable that is an instance of a subclass of
perfmon_t and pass its name to the constructor. It is not safe to create a perfmon_t
after the server starts up because the global list is not thread-safe. */

class perfmon_t : public intrusive_list_node_t<perfmon_t> {
public:
    perfmon_collection_t *parent;
    bool insert;
public:
    explicit perfmon_t(perfmon_collection_t *parent, bool insert = true);
    virtual ~perfmon_t();

    /* To get a value from a given perfmon: Call begin_stats(). On each core, call the visit_stats()
    method with the pointer that was returned from begin_stats(). Then call end_stats() on the
    pointer on the same core that you called begin_stats() on.

    You usually want to call perfmon_get_stats() instead of calling these methods directly. */
    virtual void *begin_stats() = 0;
    virtual void visit_stats(void *) = 0;
    virtual void end_stats(void *, perfmon_result_t *) = 0;
};

/* When `global_full_perfmon` is true, some perfmons will perform more elaborate stat
calculations, which might take longer but will produce more informative performance
stats. The command-line flag `--full-perfmon` sets `global_full_perfmon` to true. */

extern bool global_full_perfmon;

// Abstract perfmon subclass that implements perfmon tracking by combining per-thread values.
template<typename thread_stat_t, typename combined_stat_t = thread_stat_t>
struct perfmon_perthread_t : public perfmon_t {
    explicit perfmon_perthread_t(perfmon_collection_t *parent) : perfmon_t(parent) { }

    void *begin_stats() {
        return new thread_stat_t[get_num_threads()];
    }
    void visit_stats(void *data) {
        get_thread_stat(&(reinterpret_cast<thread_stat_t *>(data))[get_thread_id()]);
    }
    void end_stats(void *data, perfmon_result_t *dest) {
        combined_stat_t combined = combine_stats(reinterpret_cast<thread_stat_t *>(data));
        output_stat(combined, dest);
        delete[] reinterpret_cast<thread_stat_t *>(data);
    }

protected:
    virtual void get_thread_stat(thread_stat_t *) = 0;
    virtual combined_stat_t combine_stats(thread_stat_t *) = 0;
    virtual void output_stat(const combined_stat_t &, perfmon_result_t *) = 0;
};

/* perfmon_counter_t is a perfmon_t that keeps a global counter that can be incremented
and decremented. (Internally, it keeps many individual counters for thread-safety.) */
// TODO (rntz) does having the values when collecting stats be cache-padded matter?
class perfmon_counter_t : public perfmon_perthread_t<cache_line_padded_t<int64_t>, int64_t> {
    friend class perfmon_counter_step_t;
protected:
    typedef cache_line_padded_t<int64_t> padded_int64_t;
    std::string name;
    padded_int64_t *thread_data;

    //padded_int64_t thread_data[MAX_THREADS];
    int64_t &get();

    void get_thread_stat(padded_int64_t *);
    int64_t combine_stats(padded_int64_t *);
    void output_stat(const int64_t&, perfmon_result_t *);
public:
    perfmon_counter_t(const std::string& name, perfmon_collection_t *parent);
    virtual ~perfmon_counter_t();
    void operator++() { get()++; }
    void operator+=(int64_t num) { get() += num; }
    void operator--() { get()--; }
    void operator-=(int64_t num) { get() -= num; }
};

/* perfmon_sampler_t is a perfmon_t that keeps a log of events that happen. When something
happens, call the perfmon_sampler_t's record() method. The perfmon_sampler_t will retain that
record until 'length' ticks have passed. It will produce stats for the number of records in the
time period, the average record, and the min and max records. */

// need to use a namespace, not inner classes, so we can pass the auxiliary
// classes to the templated base classes
namespace perfmon_sampler {

struct stats_t {
    int count;
    double sum, min, max;
    stats_t() : count(0), sum(0),
        min(std::numeric_limits<double>::max()),
        max(std::numeric_limits<double>::min())
        { }
    void record(double v) {
        count++;
        sum += v;
        if (count) {
            min = std::min(min, v);
            max = std::max(max, v);
        } else {
            min = max = v;
        }
    }
    void aggregate(const stats_t &s) {
        count += s.count;
        sum += s.sum;
        if (s.count) {
            min = std::min(min, s.min);
            max = std::max(max, s.max);
        }
    }
};

}   /* namespace perfmon_sampler */

class perfmon_sampler_t : public perfmon_perthread_t<perfmon_sampler::stats_t> {
    typedef perfmon_sampler::stats_t stats_t;
    struct thread_info_t {
        stats_t current_stats, last_stats;
        int current_interval;
    };

    thread_info_t *thread_data;

    void get_thread_stat(stats_t *);
    stats_t combine_stats(stats_t *);
    void output_stat(const stats_t&, perfmon_result_t *);

    void update(ticks_t now);

    std::string name;
    ticks_t length;
    bool include_rate;
public:
    perfmon_sampler_t(const std::string& _name, ticks_t _length, bool _include_rate, perfmon_collection_t *parent);
    virtual ~perfmon_sampler_t();
    void record(double value);
};

/* Tracks the mean and standard deviation of a sequence value in constant space
 * & time.
 */

// One-pass variance calculation algorithm/datastructure taken from
// http://www.cs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
struct stddev_t {
    stddev_t();
    stddev_t(size_t datapoints, float mean, float variance);

    void add(float value);
    size_t datapoints() const;
    float mean() const;
    float standard_deviation() const;
    float standard_variance() const;
    //stddev_t merge(const stddev_t &other);
    static stddev_t combine(size_t nelts, stddev_t *data);

private:
    // N is the number of datapoints, M is the current mean, Q/N is
    // the standard variance, and sqrt(Q/N) is the standard
    // deviation. Read the paper for why it works or use algebra.
    //
    // (Note that this is a better scheme numerically than using the
    // classic calculator technique of storing sum(x), sum(x^2), n.)
    size_t N;
    float M, Q;
};

struct perfmon_stddev_t : public perfmon_perthread_t<stddev_t> {
    // should be possible to make this a templated class if necessary
    perfmon_stddev_t(const std::string& name, perfmon_collection_t *parent);
    void record(float value);

protected:
    std::string name;

    void get_thread_stat(stddev_t *);
    stddev_t combine_stats(stddev_t *);
    void output_stat(const stddev_t&, perfmon_result_t *);
private:
    stddev_t thread_data[MAX_THREADS]; // TODO (rntz) should this be cache-line padded?
};

/* `perfmon_rate_monitor_t` keeps track of the number of times some event happens
per second. It is different from `perfmon_sampler_t` in that it does not associate
a number with each event, but you can record many events at once. For example, it
would be good for recording how fast bytes are sent over the network. */

class perfmon_rate_monitor_t : public perfmon_perthread_t<double> {
private:
    struct thread_info_t {
        double current_count, last_count;
        int current_interval;
    } thread_data[MAX_THREADS];
    void update(ticks_t now);
    std::string name;
    ticks_t length;

    void get_thread_stat(double *);
    double combine_stats(double *);
    void output_stat(const double&, perfmon_result_t *);
public:
    perfmon_rate_monitor_t(const std::string& name, ticks_t length, perfmon_collection_t *parent);
    void record(double value = 1.0);
};

/* perfmon_function_t is a perfmon for calling an arbitrary function to compute a stat. You should
not create such a perfmon at runtime; instead, declare it as a static variable and then construct
a perfmon_function_t::internal_function_t instance for it. This way things get called on the
right cores and if there are multiple internal_function_t instances the perfmon_function_t can
combine them by inserting commas. */
struct perfmon_function_t : public perfmon_t {
public:
    class internal_function_t : public intrusive_list_node_t<internal_function_t> {
    public:
        explicit internal_function_t(perfmon_function_t *p);
        virtual ~internal_function_t();
        virtual std::string compute_stat() = 0;
    private:
        perfmon_function_t *parent;
    };

private:
    friend class internal_function_t;
    std::string name;
    intrusive_list_t<internal_function_t> funs[MAX_THREADS];

public:
    perfmon_function_t(std::string _name, perfmon_collection_t *parent)
        : perfmon_t(parent), name(_name) { }
    virtual ~perfmon_function_t() { }

    void *begin_stats();
    void visit_stats(void *data);
    void end_stats(void *data, perfmon_result_t *dest);
};

/* A perfmon collection allows you to add hierarchy to stats. */
class perfmon_collection_t : public perfmon_t {
public:
    perfmon_collection_t(const std::string &_name, perfmon_collection_t *parent, bool insert, bool _create_submap)
        : perfmon_t(parent, insert), name(_name), create_submap(_create_submap)
    {
    }

    /* Perfmon interface */
    void *begin_stats() {
        constituents_access.co_lock(rwi_read); // FIXME that should be scoped somehow, so that we unlock in case the results collection fail
        void **contexts = new void *[constituents.size()];
        size_t i = 0;
        for (perfmon_t *p = constituents.head(); p; p = constituents.next(p), ++i) {
            contexts[i] = p->begin_stats();
        }
        return contexts;
    }

    void visit_stats(void *_contexts) {
        void **contexts = reinterpret_cast<void **>(_contexts);
        size_t i = 0;
        for (perfmon_t *p = constituents.head(); p; p = constituents.next(p), ++i) {
            p->visit_stats(contexts[i]);
        }
    }

    void end_stats(void *_contexts, perfmon_result_t *result) {
        void **contexts = reinterpret_cast<void **>(_contexts);

        // TODO: This is completely fucked up shitty code.
        perfmon_result_t *map;
        if (create_submap) {
            perfmon_result_t::alloc_map_result(&map);
            rassert(result->get_map()->count(name) == 0);
            result->get_map()->insert(std::pair<std::string, perfmon_result_t *>(name, map));
        } else {
            map = result;
        }

        size_t i = 0;
        for (perfmon_t *p = constituents.head(); p; p = constituents.next(p), ++i) {
            p->end_stats(contexts[i], map);
        }

        delete[] contexts;
        constituents_access.unlock();
    }

    /* Ways to add perfmons */
    void add(perfmon_t *perfmon) {
        rwi_lock_t::write_acq_t write_acq(&constituents_access);
        constituents.push_back(perfmon);
    }

    void remove(perfmon_t *perfmon) {
        rwi_lock_t::write_acq_t write_acq(&constituents_access);
        constituents.remove(perfmon);
    }

private:
    rwi_lock_t constituents_access;

    std::string name;
    bool create_submap;
    intrusive_list_t<perfmon_t> constituents;
};

/* perfmon_duration_sampler_t is a perfmon_t that monitors events that have a
 * starting and ending time. When something starts, call begin(); when
 * something ends, call end() with the same value as begin. It will produce
 * stats for the number of active events, the average length of an event, and
 * so on. If `global_full_perfmon` is false, it won't report any timing-related
 * stats because `get_ticks()` is rather slow.
 *
 * Frequently we're in the case
 * where we'd like to have a single slow perfmon up, but don't want the other
 * ones, perfmon_duration_sampler_t has an ignore_global_full_perfmon
 * field on it, which when true makes it run regardless of --full-perfmon flag
 * this can also be enable and disabled at runtime. */

struct perfmon_duration_sampler_t {
private:
    perfmon_collection_t stat;
    perfmon_counter_t active;
    perfmon_counter_t total;
    perfmon_sampler_t recent;
    bool ignore_global_full_perfmon;
public:
    perfmon_duration_sampler_t(const std::string& name, ticks_t length, perfmon_collection_t *parent, bool _ignore_global_full_perfmon = false);
    void begin(ticks_t *v);
    void end(ticks_t *v);

public:
    //Control interface used for enabling and disabling duration samplers at run time
    std::string call(UNUSED int argc, UNUSED char **argv);
};

struct block_pm_duration {
    ticks_t time;
    bool ended;
    perfmon_duration_sampler_t *pm;
    explicit block_pm_duration(perfmon_duration_sampler_t *_pm)
        : ended(false), pm(_pm) {
        pm->begin(&time);
    }
    void end() {
        rassert(!ended);
        ended = true;
        pm->end(&time);
    }
    ~block_pm_duration() {
        if (!ended) end();
    }
};

perfmon_collection_t &get_global_perfmon_collection();

#endif /* PERFMON_HPP_ */
