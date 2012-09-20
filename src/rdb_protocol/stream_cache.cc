#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/stream_cache.hpp"

bool stream_cache_t::contains(int64_t key) {
    return streams.find(key) != streams.end();
}

void stream_cache_t::insert(ReadQuery *r, int64_t key,
                            boost::shared_ptr<query_language::json_stream_t> val) {
    maybe_evict();
    rassert(!contains(key));
    streams.insert(std::pair<int64_t, entry_t>(key, entry_t(time(0), val, r)));
}

void stream_cache_t::erase(int64_t key) {
    rassert(contains(key));
    streams.erase(streams.find(key));
}

bool stream_cache_t::serve(int64_t key, Response *res, signal_t *interruptor) {
    std::map<int64_t, entry_t>::iterator it = streams.find(key);
    if (it == streams.end()) return false;
    entry_t *entry = &it->second;
    entry->last_activity = time(0);
    try {
        int chunk_size = 0;
        // This is a hack.  Some streams have an interruptor that is invalid by
        // the time we reach here, so we just reset it to a good one.
        entry->stream->reset_interruptor(interruptor);
        while (boost::shared_ptr<scoped_cJSON_t> json = entry->stream->next()) {
            res->add_response(json->PrintUnformatted());
            if (entry->max_chunk_size && ++chunk_size >= entry->max_chunk_size) {
                debugf("PARTIAL\n");
                res->set_status_code(Response::SUCCESS_PARTIAL);
                return true;
            }
        }
    } catch (const std::exception &e) {
        erase(key);
        throw;
    }
    erase(key);
    res->set_status_code(Response::SUCCESS_STREAM);
    return true;
}

void stream_cache_t::maybe_evict() {
    time_t cur_time = time(0);
    std::map<int64_t, entry_t>::iterator it_old, it = streams.begin();
    while (it != streams.end()) {
        it_old = it++;
        entry_t *entry = &it_old->second;
        if (entry->max_age && cur_time - entry->last_activity > entry->max_age) {
            streams.erase(it_old);
        }
    }
}

/*******************************************************************************
                                    ENTRY_T
*******************************************************************************/
bool valid_chunk_size(int64_t chunk_size) {
    return 0 <= chunk_size && chunk_size <= INT_MAX;
}
bool valid_age(int64_t age) { return 0 <= age; }
stream_cache_t::entry_t::entry_t(time_t _last_activity,
                                 boost::shared_ptr<query_language::json_stream_t> _stream,
                                 ReadQuery *r)
    : last_activity(_last_activity), stream(_stream),
      max_chunk_size(DEFAULT_MAX_CHUNK_SIZE), max_age(DEFAULT_MAX_AGE) {
    if (r) {
        if (r->has_max_chunk_size() && valid_chunk_size(r->max_chunk_size())) {
            max_chunk_size = r->max_chunk_size();
        }
        if (r->has_max_age() && valid_age(max_age)) {
            max_age = r->max_age();
        }
    }
}
