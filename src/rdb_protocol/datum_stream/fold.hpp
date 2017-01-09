#ifndef RDB_PROTOCOL_DATUM_STREAM_FOLD_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_FOLD_HPP_

#include "rdb_protocol/datum_stream.hpp"

namespace ql {

class fold_datum_stream_t : public eager_datum_stream_t {
public:
    fold_datum_stream_t(counted_t<datum_stream_t> &&stream,
                        datum_t base,
                        counted_t<const func_t> &&_acc_func,
                        counted_t<const func_t> &&_emit_func,
                        counted_t<const func_t> &&_final_emit_func,
                        backtrace_id_t bt);

    std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    bool is_array() const final {
        return is_array_fold;
    }

    bool is_exhausted() const final;

    feed_type_t cfeed_type() const final {
        return union_type;
    }

    bool is_infinite() const final {
        return is_infinite_fold;
    }

private:
    counted_t<datum_stream_t> stream;
    counted_t<const func_t> acc_func;
    counted_t<const func_t> emit_func;
    counted_t<const func_t> final_emit_func;
    feed_type_t union_type;
    bool is_array_fold, is_infinite_fold;

    datum_t acc;
    bool do_final_emit;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_FOLD_HPP_
