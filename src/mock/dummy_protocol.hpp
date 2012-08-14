#ifndef MOCK_DUMMY_PROTOCOL_HPP_
#define MOCK_DUMMY_PROTOCOL_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include "backfill_progress.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/rwi_lock.hpp"
#include "protocol_api.hpp"
#include "rpc/serialize_macros.hpp"
#include "timestamps.hpp"
#include "perfmon/types.hpp"
#include "utils.hpp"

class signal_t;
class io_backender_t;

namespace mock {

class dummy_protocol_t {
public:
    class region_t {
    public:
        static region_t empty() THROWS_NOTHING;
        static region_t universe() THROWS_NOTHING;
        region_t() THROWS_NOTHING;
        region_t(char x, char y) THROWS_NOTHING;

        std::set<std::string> keys;
        bool operator<(const region_t &other) const;

        RDB_MAKE_ME_SERIALIZABLE_1(keys);
    };

    class temporary_cache_t {
        /* Dummy protocol doesn't need to cache anything */
    };

    class read_response_t {
    public:
        RDB_MAKE_ME_SERIALIZABLE_1(values);

        std::map<std::string, std::string> values;
    };

    class read_t {
    public:
        region_t get_region() const;
        read_t shard(region_t region) const;
        void unshard(std::vector<read_response_t> resps, read_response_t *response, temporary_cache_t *cache) const;
        void multistore_unshard(const std::vector<read_response_t>& resps, read_response_t *response, temporary_cache_t *cache) const;

        RDB_MAKE_ME_SERIALIZABLE_1(keys);
        region_t keys;
    };

    class write_response_t {
    public:
        RDB_MAKE_ME_SERIALIZABLE_1(old_values);
        std::map<std::string, std::string> old_values;
    };

    class write_t {
    public:
        region_t get_region() const;
        write_t shard(region_t region) const;
        void unshard(std::vector<write_response_t> resps, write_response_t *response, temporary_cache_t *cache) const;
        void multistore_unshard(const std::vector<write_response_t>& resps, write_response_t *response, temporary_cache_t *cache) const;

        RDB_MAKE_ME_SERIALIZABLE_1(values);
        std::map<std::string, std::string> values;
    };

    class backfill_chunk_t {
    public:
        std::string key, value;
        state_timestamp_t timestamp;

        region_t get_region() const THROWS_NOTHING {
            region_t r;
            r.keys.insert(key);
            return r;
        }

        backfill_chunk_t shard(DEBUG_ONLY_VAR const region_t &r) const THROWS_NOTHING {
            rassert(r.keys.find(key) != r.keys.end());
            return *this;
        }

        RDB_MAKE_ME_SERIALIZABLE_3(key, value, timestamp);
    };

    struct backfill_progress_t : public traversal_progress_t {
        explicit backfill_progress_t(int specified_home_thread) : traversal_progress_t(specified_home_thread) { }
        progress_completion_fraction_t guess_completion() const {
            return progress_completion_fraction_t::make_invalid();
        }
    };

    static region_t cpu_sharding_subspace(int subregion_number, int num_cpu_shards);


    class store_t : public store_view_t<dummy_protocol_t> {
    public:
        typedef region_map_t<dummy_protocol_t, binary_blob_t> metainfo_t;

        store_t();
        store_t(io_backender_t *io_backender, const std::string& filename, bool create, perfmon_collection_t *collection = NULL);
        ~store_t();

        void new_read_token(scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token_out) THROWS_NOTHING;
        void new_write_token(scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token_out) THROWS_NOTHING;

        void do_get_metainfo(order_token_t order_token,
                             scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
                             signal_t *interruptor,
                             metainfo_t *out) THROWS_ONLY(interrupted_exc_t);

        void set_metainfo(const metainfo_t &new_metainfo,
                          order_token_t order_token,
                          scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
                          signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

        void read(DEBUG_ONLY(const metainfo_checker_t<dummy_protocol_t>& metainfo_checker, )
                  const dummy_protocol_t::read_t &read,
                  dummy_protocol_t::read_response_t *response,
                  order_token_t order_token,
                  scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
                  signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

        void write(DEBUG_ONLY(const metainfo_checker_t<dummy_protocol_t>& metainfo_checker, )
                   const metainfo_t& new_metainfo,
                   const dummy_protocol_t::write_t &write,
                   dummy_protocol_t::write_response_t *response,
                   transition_timestamp_t timestamp,
                   order_token_t order_token,
                   scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
                   signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

        bool send_backfill(const region_map_t<dummy_protocol_t, state_timestamp_t> &start_point,
                           send_backfill_callback_t<dummy_protocol_t> *send_backfill_cb,
                           backfill_progress_t *progress,
                           scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
                           signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

        void receive_backfill(const dummy_protocol_t::backfill_chunk_t &chunk,
                              scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
                              signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

        void reset_data(const dummy_protocol_t::region_t &subregion,
                        const metainfo_t &new_metainfo,
                        scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
                        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

        std::map<std::string, std::string> values;
        std::map<std::string, state_timestamp_t> timestamps;

        metainfo_t metainfo;

    private:
        void initialize_empty();

        std::string filename;

        fifo_enforcer_source_t token_source;
        fifo_enforcer_sink_t token_sink;
        order_sink_t order_sink;

        rng_t rng;
    };
};

dummy_protocol_t::region_t a_thru_z_region();

std::string region_to_debug_str(dummy_protocol_t::region_t);

bool region_is_superset(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b);
dummy_protocol_t::region_t region_intersection(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b);
MUST_USE region_join_result_t region_join(const std::vector<dummy_protocol_t::region_t> &vec, dummy_protocol_t::region_t *out) THROWS_NOTHING;
std::vector<dummy_protocol_t::region_t> region_subtract_many(const dummy_protocol_t::region_t &a, const std::vector<dummy_protocol_t::region_t>& b);

bool operator==(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b);
bool operator!=(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b);

void debug_print(append_only_printf_buffer_t *buf, const dummy_protocol_t::region_t &region);
void debug_print(append_only_printf_buffer_t *buf, const dummy_protocol_t::write_t& write);
void debug_print(append_only_printf_buffer_t *buf, const dummy_protocol_t::backfill_chunk_t& chunk);

} // namespace mock

#endif /* MOCK_DUMMY_PROTOCOL_HPP_ */
