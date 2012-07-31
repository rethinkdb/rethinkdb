#include "stream_cache.hpp"
#include "rdb_protocol/exceptions.hpp"

bool stream_cache_t::contains(int64_t key) {
    return streams.find(key) != streams.end();
}

void stream_cache_t::insert(int64_t key, boost::shared_ptr<json_stream_t> val) {
    maybe_evict();
    rassert(!contains(key));
    streams.insert(std::pair<int64_t, entry_t>(key, entry_t(time(0), val)));
}

void stream_cache_t::erase(int64_t key) {
    rassert(contains(key));
    streams.erase(streams.find(key));
}

bool stream_cache_t::serve(int64_t key, Response *res) {
    std::map<int64_t, entry_t>::iterator entry = streams.find(key);
    if (entry == streams.end()) {
        return false;
    } else {
        entry->second.last_activity = time(0);
        boost::shared_ptr<json_stream_t> stream = entry->second.stream;
        try {
            int current_chunk_size = 0;
            while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                res->add_response(json->PrintUnformatted());
                if (++current_chunk_size >= MAX_CHUNK_SIZE) {
                    res->set_status_code(Response::PARTIAL);
                    return true;
                }
            }
        } catch (query_language::runtime_exc_t &e) {
            erase(key);
            throw e;
        }
        erase(key);
        return true;
    }
}

void stream_cache_t::maybe_evict() {
    time_t cur_time = time(0);
    std::map<int64_t, entry_t>::iterator it_old, it = streams.begin();
    while (it != streams.end()) {
        it_old = it++;
        if (cur_time - it_old->second.last_activity > MAX_AGE) streams.erase(it_old);
    }
}

stream_cache_t::~stream_cache_t() {
    streams.clear();
}
