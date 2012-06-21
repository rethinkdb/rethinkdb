#include "perfmon/core.hpp"
#include "arch/spinlock.hpp"

// TODO (rntz) remove lock, not necessary with perfmon static initialization restrictions

/* The var lock protects the global perfmon collection when it is being
 * modified. In theory, this should all work automagically because the
 * constructor of every perfmon_t calls get_var_lock(), causing the var lock to
 * be constructed before the first perfmon, so it is destroyed after the last
 * perfmon.
 */
static spinlock_t &get_var_lock() {
    /* To avoid static initialization fiasco */
    static spinlock_t lock;
    return lock;
}

/* Constructor and destructor register and deregister the perfmon. */
perfmon_t::perfmon_t(perfmon_collection_t *_parent, bool _insert)
    : parent(_parent), insert(_insert)
{
    if (insert) {
        // FIXME maybe get rid of this spinlock, especially when parent isn't global
        // According to jdoliner, you can't just remove this, because that breaks some unit-tests.
        // This should be investigated.
        spinlock_acq_t acq(&get_var_lock());
        if (!parent) {
            crash("Parent can't be NULL when adding a perfmon to a collection");
        } else {
            parent->add(this);
        }
    }
}

perfmon_t::~perfmon_t() {
    if (insert) {
        spinlock_acq_t acq(&get_var_lock());
        if (!parent) {
            crash("Parent can't be NULL when adding a perfmon to a collection");
        } else {
            parent->remove(this);
        }
    }
}

void *perfmon_collection_t::begin_stats() {
    constituents_access.co_lock(rwi_read); // FIXME that should be scoped somehow, so that we unlock in case the results collection fail
    void **contexts = new void *[constituents.size()];
    size_t i = 0;
    for (perfmon_t *p = constituents.head(); p; p = constituents.next(p), ++i) {
        contexts[i] = p->begin_stats();
    }
    return contexts;
}

void perfmon_collection_t::visit_stats(void *_contexts) {
    void **contexts = reinterpret_cast<void **>(_contexts);
    size_t i = 0;
    for (perfmon_t *p = constituents.head(); p; p = constituents.next(p), ++i) {
        p->visit_stats(contexts[i]);
    }
}

void perfmon_collection_t::end_stats(void *_contexts, perfmon_result_t *result) {
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

void perfmon_collection_t::add(perfmon_t *perfmon) {
    rwi_lock_t::write_acq_t write_acq(&constituents_access);
    constituents.push_back(perfmon);
}

void perfmon_collection_t::remove(perfmon_t *perfmon) {
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

    static perfmon_collection_t collection("Global", NULL, false, false);
    return collection;
}

