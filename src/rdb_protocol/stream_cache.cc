// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/stream_cache.hpp"

#include "rdb_protocol/env.hpp"

#include "debug.hpp"

namespace ql {

bool stream_cache_t::contains(int64_t key) {
    return streams.find(key) != streams.end();
}

void stream_cache_t::insert(int64_t key,
                             use_json_t use_json,
                             scoped_ptr_t<env_t> &&val_env,
                             counted_t<datum_stream_t> val_stream) {
    maybe_evict();
    std::pair<boost::ptr_map<int64_t, entry_t>::iterator, bool> res = streams.insert(
        key, new entry_t(time(0), use_json, std::move(val_env), val_stream));
    guarantee(res.second);
}

void stream_cache_t::erase(int64_t key) {
    size_t num_erased = streams.erase(key);
    guarantee(num_erased == 1);
}

bool stream_cache_t::serve(int64_t key, Response *res, signal_t *interruptor) {
    boost::ptr_map<int64_t, entry_t>::iterator it = streams.find(key);
    if (it == streams.end()) return false;
    entry_t *entry = it->second;
    entry->last_activity = time(0);

    std::exception_ptr exc;
    try {
        // Reset the env_t's interruptor to a good one before we use it.  This may be a
        // hack.  (I'd rather not have env_t be mutable this way -- could we construct
        // a new env_t instead?  Why do we keep env_t's around anymore?)
        entry->env->interruptor = interruptor;

        batch_type_t batch_type = entry->has_sent_batch
                                      ? batch_type_t::NORMAL
                                      : batch_type_t::NORMAL_FIRST;
        std::vector<counted_t<const datum_t> > ds
            = entry->stream->next_batch(
                entry->env.get(),
                batchspec_t::user(batch_type, entry->env.get()));
        entry->has_sent_batch = true;
        for (auto d = ds.begin(); d != ds.end(); ++d) {
            (*d)->write_to_protobuf(res->add_response(), entry->use_json);
        }
        if (entry->env->trace.has()) {
            entry->env->trace->as_datum()->write_to_protobuf(
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

    if (entry->stream->is_exhausted() || res->response_size() == 0) {
        erase(key);
        res->set_type(Response::SUCCESS_SEQUENCE);
    } else {
        res->set_type(Response::SUCCESS_PARTIAL);
    }
    return true;
}

void stream_cache_t::maybe_evict() {
    // We never evict right now.
}

stream_cache_t::entry_t::entry_t(time_t _last_activity,
                                  use_json_t _use_json,
                                  scoped_ptr_t<env_t> &&env_ptr,
                                  counted_t<datum_stream_t> _stream)
    : last_activity(_last_activity),
      use_json(_use_json),
      env(std::move(env_ptr)),
      stream(_stream),
      max_age(DEFAULT_MAX_AGE),
      has_sent_batch(false) { }

stream_cache_t::entry_t::~entry_t() { }


} // namespace ql
