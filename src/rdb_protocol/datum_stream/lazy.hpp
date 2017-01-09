#ifndef RDB_PROTOCOL_DATUM_STREAM_LAZY_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_LAZY_HPP_

#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/datum_stream/readers.hpp"

namespace ql {

class lazy_datum_stream_t : public datum_stream_t {
public:
    lazy_datum_stream_t(
        scoped_ptr_t<reader_t> &&_reader,
        backtrace_id_t bt);

    virtual bool is_array() const { return false; }
    virtual datum_t as_array(UNUSED env_t *env) {
        return datum_t();  // Cannot be converted implicitly.
    }

    bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const;
    virtual bool is_infinite() const;

    virtual bool add_stamp(changefeed_stamp_t stamp) {
        return reader->add_stamp(std::move(stamp));
    }
    virtual optional<active_state_t> get_active_state() {
        return reader->get_active_state();
    }

private:
    virtual std::vector<changespec_t> get_changespecs() {
        return std::vector<changespec_t>{changespec_t(
                reader->get_changespec(), counted_from_this())};
    }

    std::vector<datum_t >
    next_batch_impl(env_t *env, const batchspec_t &batchspec);

    virtual void add_transformation(transform_variant_t &&tv,
                                    backtrace_id_t bt);
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    // We use these to cache a batch so that `next` works.  There are a lot of
    // things that are most easily written in terms of `next` that would
    // otherwise have to do this caching themselves.
    size_t current_batch_offset;
    std::vector<datum_t> current_batch;

    scoped_ptr_t<reader_t> reader;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_LAZY_HPP_

