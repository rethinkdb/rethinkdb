// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_STREAM_CACHE_HPP_
#define RDB_PROTOCOL_STREAM_CACHE_HPP_

#include <time.h>

#include <map>
#include <string>

#include "concurrency/signal.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {
class env_t;
}

namespace ql {

class stream_cache_t {
public:
    stream_cache_t(rdb_context_t *_rdb_ctx)
        : rdb_ctx(_rdb_ctx) {
        rassert(rdb_ctx != NULL);
    }
    MUST_USE bool contains(int64_t key);
    void insert(int64_t key,
                use_json_t use_json,
                std::map<std::string, wire_func_t> global_optargs,
                profile_bool_t profile_requested,
                counted_t<datum_stream_t> val_stream);
    void erase(int64_t key);
    MUST_USE bool serve(int64_t key, Response *res, signal_t *interruptor);
private:
    void maybe_evict();

    struct entry_t {
        ~entry_t();
        static const time_t DEFAULT_MAX_AGE = 0; // 0 = never evict
        entry_t(time_t _last_activity,
                use_json_t use_json,
                std::map<std::string, wire_func_t> global_optargs,
                profile_bool_t profile,
                counted_t<datum_stream_t> _stream);
        time_t last_activity;
        use_json_t use_json;
        std::map<std::string, wire_func_t> global_optargs;
        profile_bool_t profile;
        counted_t<datum_stream_t> stream;
        time_t max_age;
        bool has_sent_batch;
    private:
        DISABLE_COPYING(entry_t);
    };

    rdb_context_t *const rdb_ctx;
    std::map<int64_t, scoped_ptr_t<entry_t> > streams;
    DISABLE_COPYING(stream_cache_t);
};

} // namespace ql

#endif  // RDB_PROTOCOL_STREAM_CACHE_HPP_
