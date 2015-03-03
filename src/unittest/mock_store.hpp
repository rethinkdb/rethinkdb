#ifndef UNITTEST_MOCK_STORE_HPP_
#define UNITTEST_MOCK_STORE_HPP_

#include <map>
#include <utility>
#include <string>

#include "rdb_protocol/protocol.hpp"
#include "store_view.hpp"

namespace unittest {

write_t mock_overwrite(std::string key, std::string value);

read_t mock_read(std::string key);

std::string mock_parse_read_response(const read_response_t &rr);

std::string mock_lookup(store_view_t *store, std::string key);

class mock_store_t : public store_view_t {
public:
    explicit mock_store_t(binary_blob_t universe_metainfo = binary_blob_t());
    ~mock_store_t();
    void rethread(threadnum_t new_thread) {
        home_thread_mixin_t::real_home_thread = new_thread;
        token_source_.rethread(new_thread);
        token_sink_.rethread(new_thread);
        order_sink_.rethread(new_thread);
    }

    void note_reshard() { }

    void new_read_token(read_token_t *token_out);
    void new_write_token(write_token_t *token_out);

    void do_get_metainfo(order_token_t order_token,
                         read_token_t *token,
                         signal_t *interruptor,
                         region_map_t<binary_blob_t> *out) THROWS_ONLY(interrupted_exc_t);

    void set_metainfo(const region_map_t<binary_blob_t> &new_metainfo,
                      order_token_t order_token,
                      write_token_t *token,
                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    void read(
            DEBUG_ONLY(const metainfo_checker_t &metainfo_checker, )
            const read_t &read,
            read_response_t *response,
            read_token_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void write(
            DEBUG_ONLY(const metainfo_checker_t &metainfo_checker, )
            const region_map_t<binary_blob_t> &new_metainfo,
            const write_t &write,
            write_response_t *response,
            write_durability_t durability,
            state_timestamp_t timestamp,
            order_token_t order_token,
            write_token_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    bool send_backfill(
            const region_map_t<state_timestamp_t> &start_point,
            send_backfill_callback_t *send_backfill_cb,
            traversal_progress_combiner_t *progress,
            read_token_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void receive_backfill(
            const backfill_chunk_t &chunk,
            write_token_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void throttle_backfill_chunk(signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void reset_data(
            const binary_blob_t &zero_version,
            const region_t &subregion,
            write_durability_t durability,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    // Used by unit tests that expected old-style stuff.
    std::string values(std::string key);
    repli_timestamp_t timestamps(std::string key);

private:
    fifo_enforcer_source_t token_source_;
    fifo_enforcer_sink_t token_sink_;

    order_sink_t order_sink_;

    rng_t rng_;
    region_map_t<binary_blob_t> metainfo_;
    std::map<store_key_t, std::pair<repli_timestamp_t, ql::datum_t> > table_;

    DISABLE_COPYING(mock_store_t);
};

}  // namespace unittest

#endif  // UNITTEST_MOCK_STORE_HPP_
