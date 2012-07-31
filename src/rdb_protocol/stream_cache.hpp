#ifndef RDB_PROTOCOL_STREAM_CACHE_HPP
#define RDB_PROTOCOL_STREAM_CACHE_HPP

#include "utils.hpp"
#include <boost/shared_ptr.hpp>
#include <map>
#include <time.h>

#include "rdb_protocol/json.hpp"
#include "rdb_protocol/query_language.pb.h"

class json_stream_t {
public:
    virtual boost::shared_ptr<scoped_cJSON_t> next() = 0; //MAY THROW
    virtual ~json_stream_t() { }
};

class stream_cache_t {
public:
    ~stream_cache_t();
    bool contains(int64_t key);
    void insert(int64_t key, boost::shared_ptr<json_stream_t> val);
    void erase(int64_t key);
    bool serve(int64_t key, Response *res);
private:
    static const int MAX_CHUNK_SIZE = 10;
    static const time_t MAX_AGE = 60;
    void maybe_evict();

    struct entry_t {
        entry_t(time_t _last_activity, boost::shared_ptr<json_stream_t> _stream) :
            last_activity(_last_activity), stream(_stream) { }
        time_t last_activity;
        boost::shared_ptr<json_stream_t> stream;
    };
    std::map<int64_t, entry_t> streams;
};

#endif
