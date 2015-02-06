// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/stream_cache.hpp"

#include "rdb_protocol/env.hpp"

#include "debug.hpp"

namespace ql {

bool stream_cache_t::contains(int64_t key) {
    rwlock_acq_t lock(&streams_lock, access_t::read);
    auto it = streams.find(key);
    return it != streams.end();
}

bool stream_cache_t::insert(int64_t key,
                            use_json_t use_json,
                            std::map<std::string, wire_func_t> global_optargs,
                            profile_bool_t profile_requested,
                            counted_t<datum_stream_t> val_stream) {
    debugf("insert %ld\n", key);
    auto entry = make_counted<entry_t>(
        time(0), use_json, std::move(global_optargs), profile_requested, val_stream);
    rwlock_acq_t lock(&streams_lock, access_t::write);
    debugf("insert locked\n");
    auto res = streams.insert(std::make_pair(key, std::move(entry)));
    return res.second;
}

bool stream_cache_t::erase(int64_t key) {
    debugf("erase %ld\n", key);
    counted_t<entry_t> entry;
    {
        rwlock_acq_t lock(&streams_lock, access_t::write);
        debugf("erase locked\n");
        auto it = streams.find(key);
        if (it == streams.end()) {
            debugf("erase abort\n");
            return false;
        }
        entry = it->second;
        streams.erase(it);
    }
    debugf("erase unlocked %zu\n", counted_use_count(entry.get()));

    // Wait for all outstanding queries on this token to finish.
    new_mutex_acq_t lock(&entry->mutex);
    debugf("erase mutex\n");
    // We're the only remaining query on this token, so it's safe to return.
    guarantee(entry.unique());
    return true;
}

bool stream_cache_t::serve(int64_t key, Response *res, signal_t *interruptor) {
    debugf("serve %ld\n", key);
    counted_t<entry_t> entry;
    {
        rwlock_acq_t lock(&streams_lock, access_t::read);
        debugf("serve locked\n");
        auto it = streams.find(key);
        if (it == streams.end()) return false;
        entry = it->second;
        entry->last_activity = time(0);
    }
    debugf("serve unlocked %zu\n", counted_use_count(entry.get()));
    new_mutex_acq_t lock(&entry->mutex);
    debugf("serve mutex\n");

    std::exception_ptr exc;
    try {
        scoped_ptr_t<profile::trace_t> trace = maybe_make_profile_trace(entry->profile);

        env_t env(rdb_ctx,
                  return_empty_normal_batches,
                  interruptor,
                  entry->global_optargs,
                  trace.get_or_null());

        batch_type_t batch_type = entry->has_sent_batch
                                      ? batch_type_t::NORMAL
                                      : batch_type_t::NORMAL_FIRST;
        std::vector<datum_t> ds
            = entry->stream->next_batch(
                &env,
                batchspec_t::user(batch_type, &env));
        entry->has_sent_batch = true;
        for (auto d = ds.begin(); d != ds.end(); ++d) {
            d->write_to_protobuf(res->add_response(), entry->use_json);
        }
        if (trace.has()) {
            trace->as_datum().write_to_protobuf(
                res->mutable_profile(), entry->use_json);
        }
    } catch (const std::exception &e) {
        exc = std::current_exception();
    }
    if (exc) {
        // We can't do this in the `catch` statement because erase may trigger
        // destructors that might need to switch coroutines to do some bookkeeping,
        // and we can't switch coroutines inside an exception handler.
        erase(key);
        std::rethrow_exception(exc);
    }

    const ql::feed_type_t cfeed = entry->stream->cfeed_type();
    if (entry->stream->is_exhausted() || (res->response_size() == 0
                                          && cfeed == feed_type_t::not_feed)) {
        erase(key);
        res->set_type(Response::SUCCESS_SEQUENCE);
    } else if (cfeed == feed_type_t::stream) {
        res->set_type(Response::SUCCESS_FEED);
    } else if (cfeed == feed_type_t::point) {
        res->set_type(Response::SUCCESS_ATOM_FEED);
    } else {
        res->set_type(Response::SUCCESS_PARTIAL);
    }
    return true;
}

stream_cache_t::entry_t::entry_t(time_t _last_activity,
                                 use_json_t _use_json,
                                 std::map<std::string, wire_func_t> _global_optargs,
                                 profile_bool_t _profile,
                                 counted_t<datum_stream_t> _stream)
    : last_activity(_last_activity),
      use_json(_use_json),
      global_optargs(std::move(_global_optargs)),
      profile(_profile),
      stream(_stream),
      max_age(DEFAULT_MAX_AGE),
      has_sent_batch(false) { }

stream_cache_t::entry_t::~entry_t() { }


} // namespace ql
