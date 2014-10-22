#include "backend.hpp"

#include "rdb_protocol/datum_stream.hpp"

class artificial_table_datum_stream_t :
    public ql::eager_datum_stream_t
{
public:
    artificial_table_datum_stream_t(
            const ql::protob_t<const Backtrace> &bt_source,
            std::vector<ql::datum_t> &&_rows) :
        ql::eager_datum_stream_t(bt_source),
        rows(std::move(_rows)), index(0) { }
private:
    ql::datum_t next(ql::env_t *env, const ql::batchspec_t &bs) {
        if (ops_to_do()) {
            return datum_stream_t::next(env, bs);
        }
        return next_impl(env);
    }

    ql::datum_t next_impl(ql::env_t *) {
        if (index < rows.size()) {
            return std::move(rows[index++]);
        } else {
            return ql::datum_t();
        }
    }

    std::vector<ql::datum_t> next_raw_batch(ql::env_t *env, const ql::batchspec_t &bs) {
        std::vector<ql::datum_t> v;
        ql::batcher_t batcher = bs.to_batcher();
        ql::datum_t d;
        while (d = next_impl(env), d.has()) {
            batcher.note_el(d);
            v.push_back(std::move(d));
            if (batcher.should_send_batch()) {
                break;
            }
        }
        return v;
    }

    bool is_exhausted() const {
        return index == rows.size();
    }

    bool is_cfeed() const { return false; }
    bool is_array() { return false; }

    std::vector<ql::datum_t> rows;
    size_t index;
};

bool artificial_table_backend_t::read_all_rows_as_stream(
        const ql::protob_t<const Backtrace> &bt,
        const datum_range_t &range,
        sorting_t sorting,
        signal_t *interruptor,
        counted_t<ql::datum_stream_t> *rows_out,
        std::string *error_out) {
    /* Fetch the rows from the backend */
    std::vector<ql::datum_t> rows;
    if (!read_all_rows_as_vector(interruptor, &rows, error_out)) {
        return false;
    }

    std::string primary_key = get_primary_key_name();

    /* Apply range filter */
    if (!range.is_universe()) {
        std::vector<ql::datum_t> temp;
        for (const ql::datum_t &row : rows) {
            ql::datum_t key = row.get_field(primary_key.c_str(), ql::NOTHROW);
            guarantee(key.has());
            if (range.contains(reql_version_t::LATEST, key)) {
                temp.push_back(row);
            }
        }
        rows = std::move(temp);
    }

    /* Apply sorting */
    if (sorting != sorting_t::UNORDERED) {
        /* It's OK to use `std::sort()` instead of `std::stable_sort()` here because
        primary keys need to be unique. If we were to support secondary indexes on
        artificial tables, we would need to ensure that `read_all_rows_as_vector()`
        returns the keys in a deterministic order and then we would need to use a
        `std::stable_sort()` here. */
        std::sort(rows.begin(), rows.end(),
            [&](const ql::datum_t &a, const ql::datum_t &b) {
                ql::datum_t a_key = a.get_field(primary_key.c_str(), ql::NOTHROW);
                ql::datum_t b_key = b.get_field(primary_key.c_str(), ql::NOTHROW);
                guarantee(a_key.has() && b_key.has());
                if (sorting == sorting_t::ASCENDING) {
                    return a_key.compare_lt(reql_version_t::LATEST, b_key);
                } else {
                    return a_key.compare_gt(reql_version_t::LATEST, b_key);
                }
            });
    }

    *rows_out = make_counted<artificial_table_datum_stream_t>(bt, std::move(rows));
    return true;
}

bool artificial_table_backend_t::read_all_rows_as_vector(
        UNUSED signal_t *interruptor,
        UNUSED std::vector<ql::datum_t> *rows_out,
        UNUSED std::string *error_out) {
    crash("Oops, the default implementation of `artificial_table_backend_t::"
          "read_all_rows_as_vector()` was called. The `artificial_table_backend_t` "
          "subclass must override at least one of `read_all_rows_as_stream()` or "
          "`read_all_rows_as_vector()`. Also, the `artificial_table_backend_t` user "
          "shouldn't ever call `read_all_rows_as_vector()` directly.");
}
