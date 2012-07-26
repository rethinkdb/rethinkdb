#include <stdarg.h>

#include "errors.hpp"
#include "logger.hpp"
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <list>

#include "arch/runtime/coroutines.hpp"
#include "containers/scoped.hpp"
#include "perfmon/core.hpp"

/* Constructor and destructor register and deregister the perfmon. */
perfmon_t::perfmon_t() {
}

perfmon_t::~perfmon_t() {
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

perfmon_collection_t::perfmon_collection_t() {
}

void *perfmon_collection_t::begin_stats() {
    stats_collection_context_t *ctx;
    {
        on_thread_t thread_switcher(home_thread());
        ctx = new stats_collection_context_t(&constituents_access, constituents.size());
    }

    size_t i = 0;
    for (perfmon_membership_t *p = constituents.head(); p; p = constituents.next(p), ++i) {
        ctx->contexts[i] = p->get()->begin_stats();
    }
    return ctx;
}

void perfmon_collection_t::visit_stats(void *_context) {
    stats_collection_context_t *ctx = reinterpret_cast<stats_collection_context_t*>(_context);
    size_t i = 0;
    for (perfmon_membership_t *p = constituents.head(); p; p = constituents.next(p), ++i) {
        p->get()->visit_stats(ctx->contexts[i]);
    }
}

perfmon_result_t * perfmon_collection_t::end_stats(void *_context) {
    stats_collection_context_t *ctx = reinterpret_cast<stats_collection_context_t*>(_context);

    perfmon_result_t *map;
    perfmon_result_t::alloc_map_result(&map);

    size_t i = 0;
    for (perfmon_membership_t *p = constituents.head(); p; p = constituents.next(p), ++i) {
        perfmon_result_t * stat = p->get()->end_stats(ctx->contexts[i]);
        if (p->splice()) {
            stat->splice_into(map);
            delete stat; // `stat` is empty now, we can delete it safely
        } else {
            map->insert(p->name, stat);
        }
    }

    {
        on_thread_t thread_switcher(home_thread());
        delete ctx; // cleans up, unlocks
    }

    return map;
}

void perfmon_collection_t::add(perfmon_membership_t *perfmon) {
    scoped_ptr_t<on_thread_t> thread_switcher;
    if (coroutines_have_been_initialized()) {
        thread_switcher.init(new on_thread_t(home_thread()));
    }

    rwi_lock_t::write_acq_t write_acq(&constituents_access);
    constituents.push_back(perfmon);
}

void perfmon_collection_t::remove(perfmon_membership_t *perfmon) {
    scoped_ptr_t<on_thread_t> thread_switcher;
    if (coroutines_have_been_initialized()) {
        thread_switcher.init(new on_thread_t(home_thread()));
    }

    rwi_lock_t::write_acq_t write_acq(&constituents_access);
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

perfmon_multi_membership_t::perfmon_multi_membership_t(perfmon_collection_t *collection, perfmon_t *perfmon, const char *name, ...) {
    // Create membership for the first provided perfmon first
    memberships.push_back(new perfmon_membership_t(collection, perfmon, name));

    va_list args;
    va_start(args, name);

    // Now go through varargs list until we read NULL
    while ((perfmon = va_arg(args, perfmon_t *)) != NULL) {
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

/* Construct a filter from a set of paths.  Paths are of the form foo/bar/baz,
   where each of those can be a regular expression.  They act a lot like XPath
   expressions, but for perfmon_t objects.  */
perfmon_filter_t::perfmon_filter_t(const std::set<std::string> &paths) {
    typedef boost::escaped_list_separator<char> separator_t;
    typedef boost::tokenizer<separator_t> tokenizer_t;
    separator_t slashes("\\","/","");

    for (std::set<std::string>::const_iterator
             str = paths.begin(); str != paths.end(); ++str) {
        regexps.push_back(std::vector<scoped_regex_t *>());
        std::vector<scoped_regex_t *> *path = &regexps.back();
        try {
            tokenizer_t t(*str, slashes);
            for (tokenizer_t::const_iterator it = t.begin(); it != t.end(); ++it) {
                path->push_back(new scoped_regex_t("^"+(*it)+"$"));
            }
        } catch (boost::escaped_list_error &e) {
            logINF("Error: Could not parse %s (%s), skipping.",
                   sanitize_for_logger(*str).c_str(), e.what());
            continue; //Skip this path
        }
    }
}

perfmon_filter_t::~perfmon_filter_t() {
    for (std::vector<std::vector<scoped_regex_t *> >::const_iterator
             it = regexps.begin(); it != regexps.end(); ++it) {
        for (std::vector<scoped_regex_t *>::const_iterator
                 regexp = it->begin(); regexp != it->end(); ++it) {
            delete *regexp;
        }
    }
}

/* Filter a [perfmon_result_t].  [depth] is how deep we are in the paths that
   the [perfmon_filter_t] was constructed from, and [active] is the set of paths
   that are still active (i.e. that haven't failed a match yet).  This should
   only be called by [filter]. */
perfmon_result_t *perfmon_filter_t::subfilter
(perfmon_result_t *p, size_t depth, std::vector<bool> active) const {
    if (p->is_string()) {
        std::string *str = p->get_string();
        for (size_t i = 0; i < regexps.size(); ++i) {
            if (!active[i]) continue;
            if (depth >= regexps[i].size()) return p;
            if (depth == regexps[i].size() && regexps[i][depth]->matches(*str)) return p;
        }
        delete p;
        return 0;
    }

    rassert(p->is_map());
    std::list<perfmon_result_t::iterator> to_delete;
    for (perfmon_result_t::iterator it = p->begin(); it != p->end(); ++it) {
        std::vector<bool> subactive = active;
        bool some_subpath = false;
        for (size_t i = 0; i < regexps.size(); ++i) {
            if (!active[i]) continue;
            if (depth >= regexps[i].size()) return p;
            subactive[i] = regexps[i][depth]->matches(it->first);
            some_subpath |= subactive[i];
        }
        if (some_subpath) it->second = subfilter(it->second, depth+1, subactive);
        if (!some_subpath || !it->second) to_delete.push_back(it);
    }
    for (std::list<perfmon_result_t::iterator>::const_iterator
             it = to_delete.begin(); it != to_delete.end(); ++it) {
        p->erase(*it);
    }
    if (p->get_map()->empty() && depth > 0) { //Never delete top node
        delete p;
        return 0;
    }
    return p;
}

perfmon_result_t *perfmon_filter_t::filter(perfmon_result_t *p) const {
    return subfilter(p, 0, std::vector<bool>(regexps.size(), true));
}

perfmon_collection_t &get_global_perfmon_collection() {
    // Getter function so that we can be sure that `collection` is initialized
    // before it is needed, as advised by the C++ FAQ. Otherwise, a `perfmon_t`
    // might be initialized before `collection` was initialized.

    // FIXME: probably use "new" to create the perfmon_collection_t. For more info check out C++ FAQ Lite answer 10.16.
    static perfmon_collection_t collection;
    return collection;
}

