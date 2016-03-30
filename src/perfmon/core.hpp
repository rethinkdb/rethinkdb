// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef PERFMON_CORE_HPP_
#define PERFMON_CORE_HPP_

#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "concurrency/cross_thread_mutex.hpp"
#include "containers/intrusive_list.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/datum.hpp"
#include "threading.hpp"

class perfmon_collection_t;
class scoped_regex_t;

/* The perfmon (short for "PERFormance MONitor") is responsible for gathering
 * data about various parts of the server.
 */

/* A perfmon_t represents a stat about the server.
 *
 * To monitor something, declare a global variable that is an instance of a
 * subclass of perfmon_t and pass its name to the constructor. It is not safe
 * to create a perfmon_t after the server starts up because the global list is
 * not thread-safe.
 */
class perfmon_t {
public:
    perfmon_t();
    virtual ~perfmon_t();

    /* To get a value from a given perfmon: Call begin_stats(). On each core,
     * call the visit_stats() method with the pointer that was returned from
     * begin_stats(). Then call end_stats() on the pointer on the same core
     * that you called begin_stats() on.

     * You usually want to call perfmon_get_stats() instead of calling these
     * methods directly.
     */
    virtual void *begin_stats() = 0;
    virtual void visit_stats(void *ctx) = 0;
    virtual ql::datum_t end_stats(void *ctx) = 0;
};

class perfmon_membership_t;

/* A perfmon collection allows you to add hierarchy to stats. */
class perfmon_collection_t : public perfmon_t, public home_thread_mixin_t {
public:
    perfmon_collection_t();
    explicit perfmon_collection_t(threadnum_t);
    ~perfmon_collection_t();

    /* Perfmon interface */
    void *begin_stats();
    void visit_stats(void *_contexts);
    ql::datum_t end_stats(void *_contexts);

private:
    friend class perfmon_membership_t;

    void add(perfmon_membership_t *perfmon);
    void remove(perfmon_membership_t *perfmon);

    // TODO: This should be a cross_thread_rwi_lock_t.
    // However concurrent reads to a perfmon_collection are currently rare,
    // and this makes the implementation simpler.
    cross_thread_mutex_t constituents_access;
    intrusive_list_t<perfmon_membership_t> constituents;
};

class perfmon_membership_t : public intrusive_list_node_t<perfmon_membership_t> {
public:
    // If the name argument is NULL or an empty string, then the perfmon
    // must be a perfmon_t that returns a map result from `end_stats` and
    // its contents will be spliced into the parent collection.
    perfmon_membership_t(perfmon_collection_t *_parent, perfmon_t *_perfmon, const char *_name, bool _own_the_perfmon = false);
    perfmon_membership_t(perfmon_collection_t *_parent, perfmon_t *_perfmon, const std::string &_name, bool _own_the_perfmon = false);
    ~perfmon_membership_t();
    perfmon_t *get();
    bool splice();

    std::string name;
private:
    perfmon_collection_t *parent;
    perfmon_t *perfmon;
    bool own_the_perfmon;

    DISABLE_COPYING(perfmon_membership_t);
};

// check_perfmon_multi_membership_args_t<...>::value is true if and only if "..." is
// comprised of the even-length sequence of types, "perfmon_t *, const char *,
// perfmon_t *, const char *, ..." (with subclasses of perfmon_t acceptable in place
// of perfmon_t).
template <class... Args>
struct check_perfmon_multi_membership_args_t;

template <class... Args>
struct flip_check_perfmon_multi_membership_args_t;

template <>
struct check_perfmon_multi_membership_args_t<> : public std::true_type { };

template <class T, class... Args>
struct check_perfmon_multi_membership_args_t<T, Args...>
    : public std::conditional<std::is_convertible<T, perfmon_t *>::value,
                              flip_check_perfmon_multi_membership_args_t<Args...>,
                              std::false_type>::type {
};

template <>
struct flip_check_perfmon_multi_membership_args_t<> : public std::false_type { };

template <class T, class... Args>
struct flip_check_perfmon_multi_membership_args_t<T, Args...>
    : public std::conditional<std::is_convertible<T, const char *>::value,
                              check_perfmon_multi_membership_args_t<Args...>,
                              std::false_type>::type {
};



class perfmon_multi_membership_t {
public:
    template <class... Args>
    perfmon_multi_membership_t(perfmon_collection_t *collection,
                               // This block of (perfmon_t *perfmon, const char
                               // *name, ) is to be repeated as many times as needed.
                               Args... args) {
        CT_ASSERT(check_perfmon_multi_membership_args_t<Args...>::value);
        init(collection, sizeof...(Args) / 2, args...);
    }

    ~perfmon_multi_membership_t();

private:
    void init(perfmon_collection_t *collection, size_t count, ...);

    std::vector<perfmon_membership_t *> memberships;

    DISABLE_COPYING(perfmon_multi_membership_t);
};

perfmon_collection_t &get_global_perfmon_collection();

#endif /* PERFMON_CORE_HPP_ */
