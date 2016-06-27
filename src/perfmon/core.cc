// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <stdarg.h>

#include "arch/runtime/coroutines.hpp"
#include "containers/scoped.hpp"
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
perfmon_collection_t::perfmon_collection_t(threadnum_t tid)
    : home_thread_mixin_t(tid),
      constituents_access(true) { }
perfmon_collection_t::~perfmon_collection_t() { }

void *perfmon_collection_t::begin_stats() {
    stats_collection_context_t *ctx =
        new stats_collection_context_t(&constituents_access, constituents);

    size_t i = 0;
    for (perfmon_membership_t *p = constituents.head(); p != nullptr; p = constituents.next(p), ++i) {
        ctx->contexts[i] = p->get()->begin_stats();
    }
    return ctx;
}

void perfmon_collection_t::visit_stats(void *_context) {
    stats_collection_context_t *ctx = reinterpret_cast<stats_collection_context_t*>(_context);
    size_t i = 0;
    for (perfmon_membership_t *p = constituents.head(); p != nullptr; p = constituents.next(p), ++i) {
        p->get()->visit_stats(ctx->contexts[i]);
    }
}

ql::datum_t perfmon_collection_t::end_stats(void *_context) {
    stats_collection_context_t *ctx = reinterpret_cast<stats_collection_context_t*>(_context);

    ql::datum_object_builder_t builder;

    size_t i = 0;
    for (perfmon_membership_t *p = constituents.head(); p != nullptr; p = constituents.next(p), ++i) {
        ql::datum_t stat = p->get()->end_stats(ctx->contexts[i]);
        if (p->splice()) {
            for (size_t j = 0; j < stat.obj_size(); ++j) {
                std::pair<datum_string_t, ql::datum_t> pair = stat.get_pair(j);
                builder.overwrite(pair.first, pair.second);
            }
        } else {
            builder.overwrite(p->name.c_str(), stat);
        }
    }
    delete ctx; // cleans up, unlocks

    return std::move(builder).to_datum();
}

void perfmon_collection_t::add(perfmon_membership_t *perfmon) {
    scoped_ptr_t<on_thread_t> thread_switcher;
    if (coroutines_have_been_initialized() && coro_t::self() != nullptr) {
        thread_switcher.init(new on_thread_t(home_thread()));
    }

    cross_thread_mutex_t::acq_t write_acq(&constituents_access);
    constituents.push_back(perfmon);
}

void perfmon_collection_t::remove(perfmon_membership_t *perfmon) {
    scoped_ptr_t<on_thread_t> thread_switcher;
    if (coroutines_have_been_initialized() && coro_t::self() != nullptr) {
        thread_switcher.init(new on_thread_t(home_thread()));
    }

    cross_thread_mutex_t::acq_t write_acq(&constituents_access);
    constituents.remove(perfmon);
}

perfmon_membership_t::perfmon_membership_t(perfmon_collection_t *_parent, perfmon_t *_perfmon, const char *_name, bool _own_the_perfmon)
    : name(_name != nullptr ? _name : ""), parent(_parent), perfmon(_perfmon), own_the_perfmon(_own_the_perfmon)
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

perfmon_collection_t &get_global_perfmon_collection() {
    // Getter function so that we can be sure that `collection` is initialized
    // before it is needed, as advised by the C++ FAQ. Otherwise, a `perfmon_t`
    // might be initialized before `collection` was initialized.

    // FIXME: probably use "new" to create the perfmon_collection_t. For more info
    // check out C++ FAQ Lite answer 10.16.
    static perfmon_collection_t collection(threadnum_t(0));
    return collection;
}

