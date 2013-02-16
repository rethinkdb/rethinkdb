// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_STREAM_CACHE_HPP_
#define RDB_PROTOCOL_STREAM_CACHE_HPP_

#include <time.h>

#include <map>

#include "utils.hpp"
#include <boost/shared_ptr.hpp>

#include "concurrency/signal.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/query_language.pb.h"
#include "rdb_protocol/rdb_protocol_json.hpp"

namespace query_language {
class json_stream_t;
}

namespace ql {
class env_t;
}

class stream_cache_t {
public:
    // TODO: Uses of contains can all just try insert or erase and look at return codes.
    bool contains(int64_t key);
    void insert(ReadQuery *r, int64_t key,
                boost::shared_ptr<query_language::json_stream_t> val);
    void erase(int64_t key);
    bool serve(int64_t key, Response *res, signal_t *interruptor);
private:
    void maybe_evict();

    struct entry_t {
        static const int DEFAULT_MAX_CHUNK_SIZE = 1000; // 0 = unbounded
        static const time_t DEFAULT_MAX_AGE = 0; // 0 = never evict
        entry_t(time_t _last_activity,
                boost::shared_ptr<query_language::json_stream_t> _stream,
                ReadQuery *r);
        time_t last_activity;
        boost::shared_ptr<query_language::json_stream_t> stream;
        int max_chunk_size; //Size of 0 = unlimited
        time_t max_age;
    };
    std::map<int64_t, entry_t> streams;
};

namespace ql {

class stream_cache2_t {
public:
    stream_cache2_t() { }
    MUST_USE bool contains(int64_t key);
    void insert(Query2 *q, int64_t key,
                scoped_ptr_t<env_t> *val_env, datum_stream_t *val_stream);
    void erase(int64_t key);
    MUST_USE bool serve(int64_t key, Response2 *res, signal_t *interruptor);
private:
    void maybe_evict();

    struct entry_t {
        ~entry_t(); // `env_t` is incomplete
        static const int DEFAULT_MAX_CHUNK_SIZE = 1000 DEBUG_ONLY(/ 200);
        static const time_t DEFAULT_MAX_AGE = 0; // 0 = never evict
        entry_t(time_t _last_activity, scoped_ptr_t<env_t> *env_ptr,
                datum_stream_t *_stream, Query2 *q);
        time_t last_activity;
        scoped_ptr_t<env_t> env; // steals ownership from env_ptr !!!
        datum_stream_t *stream;
        int max_chunk_size; // Size of 0 = unlimited
        time_t max_age;
    private:
        DISABLE_COPYING(entry_t);
    };

    boost::ptr_map<int64_t, entry_t> streams;
    DISABLE_COPYING(stream_cache2_t);
};

} // namespace ql

#endif  // RDB_PROTOCOL_STREAM_CACHE_HPP_
