#ifndef UNITTEST_MOCK_STORE_HPP_
#define UNITTEST_MOCK_STORE_HPP_

#include "rdb_protocol/protocol.hpp"

namespace unittest {

rdb_protocol_t::write_t mock_overwrite(std::string key, std::string value);

class mock_store_t : public store_view_t<rdb_protocol_t> {
public:
    mock_store_t();
    ~mock_store_t();

    void new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out);
    void new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out);

    void do_get_metainfo(order_token_t order_token,
                         object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
                         signal_t *interruptor,
                         metainfo_t *out) THROWS_ONLY(interrupted_exc_t);

    void set_metainfo(const metainfo_t &new_metainfo,
                      order_token_t order_token,
                      object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    void read(
            DEBUG_ONLY(const metainfo_checker_t<rdb_protocol_t> &metainfo_checker, )
            const rdb_protocol_t::read_t &read,
            rdb_protocol_t::read_response_t *response,
            order_token_t order_token,
            read_token_pair_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void write(
            DEBUG_ONLY(const metainfo_checker_t<rdb_protocol_t> &metainfo_checker, )
            const metainfo_t& new_metainfo,
            const rdb_protocol_t::write_t &write,
            rdb_protocol_t::write_response_t *response,
            write_durability_t durability,
            transition_timestamp_t timestamp,
            order_token_t order_token,
            write_token_pair_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    bool send_backfill(
            const region_map_t<rdb_protocol_t, state_timestamp_t> &start_point,
            send_backfill_callback_t<rdb_protocol_t> *send_backfill_cb,
            traversal_progress_combiner_t *progress,
            read_token_pair_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void receive_backfill(
            const rdb_protocol_t::backfill_chunk_t &chunk,
            write_token_pair_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void reset_data(
            const rdb_protocol_t::region_t &subregion,
            const metainfo_t &new_metainfo,
            write_token_pair_t *token,
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
    metainfo_t metainfo_;
    std::map<store_key_t, std::pair<repli_timestamp_t, counted_t<const ql::datum_t> > > table_;

    DISABLE_COPYING(mock_store_t);
};

}  // namespace unittest

#endif  // UNITTEST_MOCK_STORE_HPP_
