#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

#include "perfmon/core.hpp"
#include "arch/runtime/coroutines.hpp"

/* Constructor and destructor register and deregister the perfmon. */
perfmon_t::perfmon_t(perfmon_collection_t *_parent, bool _insert)
    : parent(_parent), insert(_insert)
{
    if (insert) {
        guarantee(_parent != NULL, "Global perfmon collection must be specified explicitly instead of using NULL");
        if (coroutines_have_been_initialized()) {
            // We spawn a coroutine which will add the current perfmon into the parent perfmon collection. We do
            // it in order to avoid adding an incompletely constructed object into the perfmon collection which
            // could be queried concurrently. It is assumed here that the constructor of the deriving class
            // cannot block. If it can, it's a bug!
            coro_t::spawn_sometime(boost::bind(&perfmon_collection_t::add, parent, this));
        } else {
            // There are no other coroutines and, we assume, no other threads that can access the parent perfmon
            // collection, so we add `this` into `parent` right now, even though the current object hasn't been
            // completely constructed.
            parent->add(this);
        }
    }
}

perfmon_t::~perfmon_t() {
    if (insert) {
        parent->remove(this);
    }
}

struct stats_collection_context_t : public home_thread_mixin_t {
    stats_collection_context_t(rwi_lock_t *constituents_lock, size_t size)
        : contexts(new void *[size]), lock_sentry(constituents_lock) { }

    ~stats_collection_context_t() {
        delete[] contexts;
    }

    void **contexts;
private:
    rwi_lock_t::read_acq_t lock_sentry;
};

void *perfmon_collection_t::begin_stats() {
    stats_collection_context_t *ctx;
    {
        on_thread_t thread_switcher(home_thread());
        ctx = new stats_collection_context_t(&constituents_access, constituents.size());
    }

    size_t i = 0;
    for (perfmon_t *p = constituents.head(); p; p = constituents.next(p), ++i) {
        ctx->contexts[i] = p->begin_stats();
    }
    return ctx;
}

void perfmon_collection_t::visit_stats(void *_context) {
    stats_collection_context_t *ctx = reinterpret_cast<stats_collection_context_t*>(_context);
    size_t i = 0;
    for (perfmon_t *p = constituents.head(); p; p = constituents.next(p), ++i) {
        p->visit_stats(ctx->contexts[i]);
    }
}

void perfmon_collection_t::end_stats(void *_context, perfmon_result_t *result) {
    stats_collection_context_t *ctx = reinterpret_cast<stats_collection_context_t*>(_context);

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
        p->end_stats(ctx->contexts[i], map);
    }

    {
        on_thread_t thread_switcher(home_thread());
        delete ctx; // cleans up, unlocks
    }
}

void perfmon_collection_t::add(perfmon_t *perfmon) {
    boost::scoped_ptr<on_thread_t> thread_switcher;
    if (coroutines_have_been_initialized()) {
        thread_switcher.reset(new on_thread_t(home_thread()));
    }

    rwi_lock_t::write_acq_t write_acq(&constituents_access);
    constituents.push_back(perfmon);
}

void perfmon_collection_t::remove(perfmon_t *perfmon) {
    boost::scoped_ptr<on_thread_t> thread_switcher;
    if (coroutines_have_been_initialized()) {
        thread_switcher.reset(new on_thread_t(home_thread()));
    }

    rwi_lock_t::write_acq_t write_acq(&constituents_access);
    constituents.remove(perfmon);
}

perfmon_result_t::perfmon_result_t() {
    type = type_value;
}

perfmon_result_t::perfmon_result_t(const perfmon_result_t &copyee)
    : type(copyee.type), value_(copyee.value_), map_() {
    for (perfmon_result_t::internal_map_t::const_iterator it = copyee.map_.begin(); it != copyee.map_.end(); ++it) {
        perfmon_result_t *subcopy = new perfmon_result_t(*it->second);
        map_.insert(std::pair<std::string, perfmon_result_t *>(it->first, subcopy));
    }
}

perfmon_result_t::perfmon_result_t(const std::string &s) {
    type = type_value;
    value_ = s;
}

perfmon_result_t::perfmon_result_t(const std::map<std::string, perfmon_result_t *> &m) {
    type = type_map;
    map_ = m;
}

perfmon_result_t::~perfmon_result_t() {
    if (type == type_map) {
        clear_map();
    }
    rassert(map_.empty());
}

void perfmon_result_t::clear_map() {
    for (perfmon_result_t::internal_map_t::iterator it = map_.begin(); it != map_.end(); ++it) {
        delete it->second;
    }
    map_.clear();
}

perfmon_result_t perfmon_result_t::make_string() {
    return perfmon_result_t(std::string());
}

void perfmon_result_t::alloc_string_result(perfmon_result_t **out) {
    *out = new perfmon_result_t(std::string());
}

perfmon_result_t perfmon_result_t::make_map() {
    return perfmon_result_t(perfmon_result_t::internal_map_t());
}

void perfmon_result_t::alloc_map_result(perfmon_result_t **out) {
    *out = new perfmon_result_t(perfmon_result_t::internal_map_t());
}

std::string *perfmon_result_t::get_string() {
    rassert(type == type_value);
    return &value_;
}

const std::string *perfmon_result_t::get_string() const {
    rassert(type == type_value);
    return &value_;
}

perfmon_result_t::internal_map_t *perfmon_result_t::get_map() {
    rassert(type == type_map);
    return &map_;
}

const perfmon_result_t::internal_map_t *perfmon_result_t::get_map() const {
    rassert(type == type_map);
    return &map_;
}

size_t perfmon_result_t::get_map_size() const {
    rassert(type == type_map);
    return map_.size();
}

bool perfmon_result_t::is_string() const {
    return type == type_value;
}

bool perfmon_result_t::is_map() const {
    return type == type_map;
}

perfmon_result_t::perfmon_result_type_t perfmon_result_t::get_type() const {
    return type;
}

void perfmon_result_t::reset_type(perfmon_result_type_t new_type) {
    value_.clear();
    clear_map();
    type = new_type;
}

std::pair<perfmon_result_t::iterator, bool> perfmon_result_t::insert(const std::string &name, perfmon_result_t *val) {
    std::string s(name);
    perfmon_result_t::internal_map_t *map = get_map();
    rassert(map->count(name) == 0);
    return map->insert(std::pair<std::string, perfmon_result_t *>(s, val));
}

perfmon_result_t::iterator perfmon_result_t::begin() {
    return map_.begin();
}

perfmon_result_t::iterator perfmon_result_t::end() {
    return map_.end();
}

perfmon_result_t::const_iterator perfmon_result_t::begin() const {
    return map_.begin();
}

perfmon_result_t::const_iterator perfmon_result_t::end() const {
    return map_.end();
}

perfmon_collection_t &get_global_perfmon_collection() {
    /* Getter function so that we can be sure that `collection` is initialized
    before it is needed, as advised by the C++ FAQ. Otherwise, a `perfmon_t`
    might be initialized before `collection` was initialized. */

    // FIXME: probably use "new" to create the perfmon_collection_t. For more info check out C++ FAQ Lite answer 10.16.
    static perfmon_collection_t collection("Global", NULL, false, false);
    return collection;
}

