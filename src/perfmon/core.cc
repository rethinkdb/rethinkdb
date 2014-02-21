// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <stdarg.h>

#include "errors.hpp"
#include <boost/tokenizer.hpp>

#include "arch/runtime/coroutines.hpp"
#include "containers/scoped.hpp"
#include "containers/scoped_regex.hpp"
#include "logger.hpp"
#include "perfmon/core.hpp"
#include "utils.hpp"

/* Constructor and destructor register and deregister the perfmon. */
perfmon_t::perfmon_t() {
}

perfmon_t::~perfmon_t() {
}

struct stats_collection_context_t : public home_thread_mixin_t {
private:
    // This could be a read lock ... if we used a read-write lock instead of a mutex.
    cross_thread_mutex_t::acq_t lock_sentry;
public:
    scoped_array_t<void *> contexts;

    stats_collection_context_t(cross_thread_mutex_t *constituents_lock,
                               const intrusive_list_t<perfmon_membership_t> &constituents) :
        lock_sentry(constituents_lock),
        contexts(new void *[constituents.size()](),
                 constituents.size()) { }

    ~stats_collection_context_t() { }
};

perfmon_collection_t::perfmon_collection_t() : constituents_access(true) { }
perfmon_collection_t::~perfmon_collection_t() { }

void *perfmon_collection_t::begin_stats() {
    stats_collection_context_t *ctx =
        new stats_collection_context_t(&constituents_access, constituents);

    size_t i = 0;
    for (perfmon_membership_t *p = constituents.head(); p != NULL; p = constituents.next(p), ++i) {
        ctx->contexts[i] = p->get()->begin_stats();
    }
    return ctx;
}

void perfmon_collection_t::visit_stats(void *_context) {
    stats_collection_context_t *ctx = reinterpret_cast<stats_collection_context_t*>(_context);
    size_t i = 0;
    for (perfmon_membership_t *p = constituents.head(); p != NULL; p = constituents.next(p), ++i) {
        p->get()->visit_stats(ctx->contexts[i]);
    }
}

scoped_ptr_t<perfmon_result_t> perfmon_collection_t::end_stats(void *_context) {
    stats_collection_context_t *ctx = reinterpret_cast<stats_collection_context_t*>(_context);

    scoped_ptr_t<perfmon_result_t> map = perfmon_result_t::alloc_map_result();

    size_t i = 0;
    for (perfmon_membership_t *p = constituents.head(); p != NULL; p = constituents.next(p), ++i) {
        scoped_ptr_t<perfmon_result_t> stat = p->get()->end_stats(ctx->contexts[i]);
        if (p->splice()) {
            stat->splice_into(map.get());
        } else {
            map->insert(p->name, stat.release());
        }
    }

    delete ctx; // cleans up, unlocks

    return map;
}

void perfmon_collection_t::add(perfmon_membership_t *perfmon) {
    scoped_ptr_t<on_thread_t> thread_switcher;
    if (coroutines_have_been_initialized() && coro_t::self() != NULL) {
        thread_switcher.init(new on_thread_t(home_thread()));
    }

    cross_thread_mutex_t::acq_t write_acq(&constituents_access);
    constituents.push_back(perfmon);
}

void perfmon_collection_t::remove(perfmon_membership_t *perfmon) {
    scoped_ptr_t<on_thread_t> thread_switcher;
    if (coroutines_have_been_initialized() && coro_t::self() != NULL) {
        thread_switcher.init(new on_thread_t(home_thread()));
    }

    cross_thread_mutex_t::acq_t write_acq(&constituents_access);
    constituents.remove(perfmon);
}

perfmon_membership_t::perfmon_membership_t(perfmon_collection_t *_parent, perfmon_t *_perfmon, const char *_name, bool _own_the_perfmon)
    : name(_name != NULL ? _name : ""), parent(_parent), perfmon(_perfmon), own_the_perfmon(_own_the_perfmon)
{
    parent->add(this);
}

perfmon_membership_t::perfmon_membership_t(perfmon_collection_t *_parent, perfmon_t *_perfmon, const std::string &_name, bool _own_the_perfmon)
    : name(_name), parent(_parent), perfmon(_perfmon), own_the_perfmon(_own_the_perfmon)
{
    parent->add(this);
}

perfmon_membership_t::~perfmon_membership_t() {
    parent->remove(this);
    if (own_the_perfmon)
        delete perfmon;
}

perfmon_t *perfmon_membership_t::get() {
    return perfmon;
}

bool perfmon_membership_t::splice() {
    return name.length() == 0;
}

void perfmon_multi_membership_t::init(perfmon_collection_t *collection,
                                      size_t count, ...) {
    va_list args;
    va_start(args, count);

    perfmon_t *perfmon;
    const char *name;
    for (size_t i = 0; i < count; ++i) {
        perfmon = va_arg(args, perfmon_t *);
        name = va_arg(args, const char *);
        memberships.push_back(new perfmon_membership_t(collection, perfmon, name));
    }

    va_end(args);
}

perfmon_multi_membership_t::~perfmon_multi_membership_t() {
    for (std::vector<perfmon_membership_t*>::const_iterator it = memberships.begin(); it != memberships.end(); ++it) {
        delete *it;
    }
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

void perfmon_result_t::erase(perfmon_result_t::iterator it) {
    delete it->second;
    map_.erase(it);
}

scoped_ptr_t<perfmon_result_t> perfmon_result_t::alloc_map_result() {
    return scoped_ptr_t<perfmon_result_t>(new perfmon_result_t(internal_map_t()));
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
    rassert(map->count(name) == 0, "Duplicate perfmons for: %s\n", name.c_str());
    return map->insert(std::pair<std::string, perfmon_result_t *>(s, val));
}

perfmon_result_t::iterator perfmon_result_t::begin() {
    return map_.begin();
}

perfmon_result_t::iterator perfmon_result_t::end() {
    return map_.end();
}

perfmon_result_t::const_iterator perfmon_result_t::begin() const {
    return map_.cbegin();
}

perfmon_result_t::const_iterator perfmon_result_t::end() const {
    return map_.cend();
}

perfmon_result_t::const_iterator perfmon_result_t::cbegin() const {
    return map_.cbegin();
}

perfmon_result_t::const_iterator perfmon_result_t::cend() const {
    return map_.cend();
}

void perfmon_result_t::splice_into(perfmon_result_t *map) {
    rassert(type == type_map);

    // Transfer all elements from the internal map to the passed map.
    // Unfortunately we can't use here std::map::insert(InputIterator first, InputIterator last),
    // because that way we can overwrite an entry in the target map and thus leak a
    // perfmon_result_t value.
    for (const_iterator it = begin(); it != end(); ++it) {
        map->insert(it->first, it->second);
    }
    map_.clear();
}

perfmon_collection_t &get_global_perfmon_collection() {
    // Getter function so that we can be sure that `collection` is initialized
    // before it is needed, as advised by the C++ FAQ. Otherwise, a `perfmon_t`
    // might be initialized before `collection` was initialized.

    // FIXME: probably use "new" to create the perfmon_collection_t. For more info check out C++ FAQ Lite answer 10.16.
    static perfmon_collection_t collection;
    return collection;
}

