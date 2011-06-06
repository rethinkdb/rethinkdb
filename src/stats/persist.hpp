#ifndef __STATS_PERSIST_HPP__
#define __STATS_PERSIST_HPP__

#include "arch/core.hpp"
#include "containers/intrusive_list.hpp"
#include "perfmon.hpp"

#include <sstream>

struct metadata_store_t {
    // TODO (rntz) should this use key_store_t and data_provider_t, etc?
    virtual bool get_meta(const std::string &key, std::string *out) = 0;
    virtual void set_meta(const std::string &key, const std::string &value) = 0;
};

// A stat about the store, persisted via btree_key_value_store_t::{get,set}_meta
struct persistent_stat_t
    : public intrusive_list_node_t<persistent_stat_t>
{
    persistent_stat_t();
    ~persistent_stat_t();

    // Invariant: a call to unpersist_all comes before the first call to persist_all
    // Invariant: always called on the same store
    static void persist_all(metadata_store_t *store);
    static void unpersist_all(metadata_store_t *store);

  protected:
    virtual std::string persist_key() = 0;
    // unpersist is not required, and should not be assumed, to be threadsafe (it should not be
    // called from multiple threads simultaneously)
    virtual void unpersist(const std::string) = 0;
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
        reset_thread_stat(&((thread_stat_t *) data)[get_thread_id()]);
    }

    std::string end_persist(void *data) {
        thread_stat_t *stats = (thread_stat_t*) data;
        std::string result = combine_persist(stats);
        delete[] stats;
        return result;
    }

  protected:
    // Should read off the thread's stat /and reset it/, since it is about to get combined &
    // persisted. Is neither required nor assumed to be threadsafe.
    virtual void reset_thread_stat(thread_stat_t *stat) = 0;
    virtual std::string combine_persist(thread_stat_t *stats) = 0;
};

#endif
