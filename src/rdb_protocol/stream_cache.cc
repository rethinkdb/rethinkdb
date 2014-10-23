// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/stream_cache.hpp"

#include "rdb_protocol/env.hpp"

#include "debug.hpp"

namespace ql {

bool stream_cache_t::contains(int64_t key) {
    return streams.find(key) != streams.end();
}

void stream_cache_t::insert(int64_t key,
                            use_json_t use_json,
                            std::map<std::string, wire_func_t> global_optargs,
                            profile_bool_t profile_requested,
                            counted_t<datum_stream_t> val_stream) {
    maybe_evict();
    auto res = streams.insert(
            std::make_pair(key,
                           make_scoped<entry_t>(time(0),
                                                use_json,
                                                std::move(global_optargs),
                                                profile_requested,
                                                val_stream)));
    guarantee(res.second);
}

void stream_cache_t::erase(int64_t key) {
    size_t num_erased = streams.erase(key);
    guarantee(num_erased == 1);
}

bool stream_cache_t::serve(int64_t key, Response *res, signal_t *interruptor) {
    std::map<int64_t, scoped_ptr_t<entry_t> >::iterator it = streams.find(key);
    if (it == streams.end()) {
        return false;
    }
    entry_t *const entry = it->second.get();
    entry->last_activity = time(0);

    std::exception_ptr exc;
    try {
        scoped_ptr_t<profile::trace_t> trace = maybe_make_profile_trace(entry->profile);

        env_t env(rdb_ctx, interruptor, entry->global_optargs, trace.get_or_null());

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

    const bool cfeed = entry->stream->is_cfeed();
    if (entry->stream->is_exhausted() || (res->response_size() == 0 && !cfeed)) {
        erase(key);
        res->set_type(Response::SUCCESS_SEQUENCE);
    } else {
        res->set_type(cfeed ? Response::SUCCESS_FEED : Response::SUCCESS_PARTIAL);
    }
    return true;
}

void stream_cache_t::maybe_evict() {
    // We never evict right now.
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
