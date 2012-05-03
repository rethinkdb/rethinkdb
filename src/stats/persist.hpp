#ifndef STATS_PERSIST_HPP_
#define STATS_PERSIST_HPP_

#include "arch/runtime/runtime.hpp"
#include "containers/intrusive_list.hpp"
#include "perfmon.hpp"


class metadata_store_t {
public:
    // TODO (rntz) should this use key_store_t and data_buffer_t, etc?
    virtual bool get_meta(const std::string &key, std::string *out) = 0;
    virtual void set_meta(const std::string &key, const std::string &value) = 0;
    virtual ~metadata_store_t() { }
};

// A stat about the store, persisted via btree_key_value_store_t::{get,set}_meta
struct persistent_stat_t
    : public intrusive_list_node_t<persistent_stat_t>
{
    persistent_stat_t();
    virtual ~persistent_stat_t();

    // Invariant: a call to unpersist_all comes before the first call to persist_all
    // Invariant: always called on the same store
    static void persist_all(metadata_store_t *store);
    static void unpersist_all(metadata_store_t *store);

protected:
    virtual std::string persist_key() = 0;
    // unpersist is not required, and should not be assumed, to be threadsafe (it should not be
    // called from multiple threads simultaneously)
    virtual void unpersist(const std::string&) = 0;
    // {begin,visit,end}_persist are used much the same as the similar-named methods on perfmons. No
    // two instances of the persistence process should be occurring at the same time (though of
    // course, visit_persist() may occur simultaneously on different threads).
    virtual void *begin_persist() = 0;
    virtual std::string end_persist(void *) = 0;
public:                       // unfortunately needs to be public for implementation reasons
    virtual void visit_persist(void *) = 0;
};

// Abstract persistent_stat subclass implemented by combining per-thread values.
template<typename thread_stat_t>
struct persistent_stat_perthread_t
    : public persistent_stat_t
{
    void *begin_persist() { return new thread_stat_t[get_num_threads()]; }

    void visit_persist(void *data) {
        get_thread_persist(&(reinterpret_cast<thread_stat_t *>(data))[get_thread_id()]);
    }

    std::string end_persist(void *data) {
        thread_stat_t *stats = reinterpret_cast<thread_stat_t *>(data);
        std::string result = combine_persist(stats);
        delete[] stats;
        return result;
    }

protected:
    virtual void get_thread_persist(thread_stat_t *stat) = 0;
    virtual std::string combine_persist(thread_stat_t *stats) = 0;
};

#define PERSISTENT_STAT_PERTHREAD_IMPL(thread_stat_t)   \
    void get_thread_persist(thread_stat_t *);           \
    std::string combine_persist(thread_stat_t *);       \
    std::string persist_key();                          \
    void unpersist(const std::string&)

// Persistent perfmon counter
struct perfmon_persistent_counter_t
    : public perfmon_counter_t
    , public persistent_stat_perthread_t<cache_line_padded_t<int64_t> >
{
    explicit perfmon_persistent_counter_t(const std::string& name, perfmon_collection_t *parent = NULL);

    PERSISTENT_STAT_PERTHREAD_IMPL(padded_int64_t);
    int64_t combine_stats(padded_int64_t *);

private:
    int64_t unpersisted_value;
};

// Persistent stddev
struct perfmon_persistent_stddev_t
    : public perfmon_stddev_t
    , public persistent_stat_perthread_t<stddev_t>
{
    explicit perfmon_persistent_stddev_t(const std::string& name, perfmon_collection_t *parent = NULL);

    PERSISTENT_STAT_PERTHREAD_IMPL(stddev_t);
    stddev_t combine_stats(stddev_t *);

private:
    stddev_t unpersisted_value;
};

#endif
