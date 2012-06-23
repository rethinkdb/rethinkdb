#ifndef PERFMON_CORE_HPP_
#define PERFMON_CORE_HPP_

#include <string>
#include <map>

#include "containers/intrusive_list.hpp"
#include "utils.hpp"
#include "concurrency/rwi_lock.hpp"

class perfmon_collection_t;
class perfmon_result_t;

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
class perfmon_t : public intrusive_list_node_t<perfmon_t> {
public:
    perfmon_collection_t *parent;
    bool insert;
public:
    explicit perfmon_t(perfmon_collection_t *parent, bool insert = true);
    virtual ~perfmon_t();

    /* To get a value from a given perfmon: Call begin_stats(). On each core,
     * call the visit_stats() method with the pointer that was returned from
     * begin_stats(). Then call end_stats() on the pointer on the same core
     * that you called begin_stats() on.

     * You usually want to call perfmon_get_stats() instead of calling these
     * methods directly.
     */
    virtual void *begin_stats() = 0;
    virtual void visit_stats(void *) = 0;
    virtual void end_stats(void *, perfmon_result_t *) = 0;
};

/* A perfmon collection allows you to add hierarchy to stats. */
class perfmon_collection_t : public perfmon_t, public home_thread_mixin_t {
public:
    perfmon_collection_t(const std::string &_name, perfmon_collection_t *parent, bool insert, bool _create_submap)
        : perfmon_t(parent, insert), name(_name), create_submap(_create_submap) { }

    /* Perfmon interface */
    void *begin_stats();
    void visit_stats(void *_contexts);
    void end_stats(void *_contexts, perfmon_result_t *result);

    void add(perfmon_t *perfmon);
    void remove(perfmon_t *perfmon);
private:
    rwi_lock_t constituents_access;

    std::string name;
    bool create_submap;
    intrusive_list_t<perfmon_t> constituents;
};

class perfmon_result_t {
public:
    typedef std::map<std::string, perfmon_result_t *> internal_map_t;
    typedef internal_map_t::iterator iterator;
    typedef internal_map_t::const_iterator const_iterator;

    enum perfmon_result_type_t {
        type_value,
        type_map,
    };

    perfmon_result_t();
    perfmon_result_t(const perfmon_result_t &);
    explicit perfmon_result_t(const std::string &);
    virtual ~perfmon_result_t();

    static perfmon_result_t make_string();
    static void alloc_string_result(perfmon_result_t **out);
    static perfmon_result_t make_map();
    static void alloc_map_result(perfmon_result_t **out);

    std::string *get_string();
    const std::string *get_string() const;

    internal_map_t *get_map();
    const internal_map_t *get_map() const;
    size_t get_map_size() const;

    bool is_string() const;
    bool is_map() const;

    perfmon_result_type_t get_type() const;
    void reset_type(perfmon_result_type_t new_type);

    std::pair<iterator, bool> insert(const std::string &name, perfmon_result_t *val);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

private:
    void clear_map();
    explicit perfmon_result_t(const internal_map_t &);

    perfmon_result_type_t type;

    std::string value_;
    internal_map_t map_;

    void operator=(const perfmon_result_t &);
};

perfmon_collection_t &get_global_perfmon_collection();

#endif /* PERFMON_CORE_HPP_ */
