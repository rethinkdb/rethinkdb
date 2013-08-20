// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_STREAM_CACHE_HPP_
#define RDB_PROTOCOL_STREAM_CACHE_HPP_

#include <time.h>

#include <map>

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "concurrency/signal.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {
class env_t;
}

namespace ql {

class stream_cache2_t {
public:
    stream_cache2_t() { }
    MUST_USE bool contains(int64_t key);
    void insert(int64_t key,
                scoped_ptr_t<env_t> *val_env, counted_t<datum_stream_t> val_stream);
    void erase(int64_t key);
    MUST_USE bool serve(int64_t key, Response *res, signal_t *interruptor);
private:
    void maybe_evict();

    struct entry_t {
        ~entry_t(); // `env_t` is incomplete
#ifndef NDEBUG
        static const int DEFAULT_MAX_CHUNK_SIZE = 5;
#else
        static const int DEFAULT_MAX_CHUNK_SIZE = 1000;
#endif // NDEBUG
        static const time_t DEFAULT_MAX_AGE = 0; // 0 = never evict
        entry_t(time_t _last_activity, scoped_ptr_t<env_t> *env_ptr,
                counted_t<datum_stream_t> _stream);
        time_t last_activity;
        scoped_ptr_t<env_t> env; // steals ownership from env_ptr !!!
        counted_t<datum_stream_t> stream;
        int max_chunk_size; // Size of 0 = unlimited
        time_t max_age;

        scoped_ptr_t<Datum> next_datum;
    private:
        DISABLE_COPYING(entry_t);
    };

    boost::ptr_map<int64_t, entry_t> streams;
    DISABLE_COPYING(stream_cache2_t);
};

} // namespace ql

#endif  // RDB_PROTOCOL_STREAM_CACHE_HPP_
