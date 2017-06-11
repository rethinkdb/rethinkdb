#ifndef RDB_PROTOCOL_DATUM_STREAM_ORDERED_UNION_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_ORDERED_UNION_HPP_

#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/order_util.hpp"

namespace ql {

class ordered_union_datum_stream_t : public eager_datum_stream_t {
public:
    ordered_union_datum_stream_t(std::vector<counted_t<datum_stream_t> > &&_streams,
                                 std::vector<
                                 std::pair<order_direction_t,
                                           counted_t<const func_t> > > &&_comparisons,
                                 env_t *env,
                                 backtrace_id_t bt);

    std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec) final;

    bool is_array() const final {
        return is_array_ordered_union;
    }

    bool is_exhausted() const final;

    feed_type_t cfeed_type() const final {
        return union_type;
    }

    bool is_infinite() const final {
        return is_infinite_ordered_union;
    }

private:
    std::deque<counted_t<datum_stream_t> > streams;

    feed_type_t union_type;
    bool is_array_ordered_union, is_infinite_ordered_union;
    bool is_ordered_by_field;

    bool do_prelim_cache;

    struct merge_cache_item_t {
        datum_t value;
        counted_t<datum_stream_t> source;
    };

    cond_t non_interruptor;
    scoped_ptr_t<env_t> merge_env;

    struct merge_less_t {
        env_t *env;
        profile::sampler_t *merge_sampler;
        lt_cmp_t *merge_lt_cmp;
        bool operator()(const merge_cache_item_t &a, const merge_cache_item_t &b);
    };

    lt_cmp_t lt;

    std::priority_queue<merge_cache_item_t,
                        std::vector<merge_cache_item_t>,
                        merge_less_t> merge_cache;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_ORDERED_UNION_HPP_
