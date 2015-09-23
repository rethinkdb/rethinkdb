#include "backend.hpp"

#include <algorithm>

#include "rdb_protocol/artificial_table/artificial_table.hpp"
#include "rdb_protocol/datum_stream.hpp"

bool artificial_table_backend_t::read_all_rows_as_stream(
        ql::backtrace_id_t bt,
        const ql::datumspec_t &datumspec,
        sorting_t sorting,
        signal_t *interruptor,
        counted_t<ql::datum_stream_t> *rows_out,
        admin_err_t *error_out) {
    /* Fetch the rows from the backend */
    std::vector<ql::datum_t> rows;
    if (!read_all_rows_as_vector(interruptor, &rows, error_out)) {
        return false;
    }

    std::string primary_key = get_primary_key_name();

    /* Apply range filter */
    if (!datumspec.is_universe()) {
        std::vector<ql::datum_t> filter_rows;
        for (auto &&row : rows) {
            ql::datum_t key = row.get_field(primary_key.c_str(), ql::NOTHROW);
            guarantee(key.has());
            for (size_t i = 0; i < datumspec.copies(key); ++i) {
                filter_rows.push_back(row);
            }
        }
        rows = std::move(filter_rows);
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
                    return a_key < b_key;
                } else {
                    return a_key > b_key;
                }
            });
    }

    ql::changefeed::keyspec_t::range_t range_keyspec;
    range_keyspec.sorting = sorting;
    range_keyspec.datumspec = datumspec;
    boost::optional<ql::changefeed::keyspec_t> keyspec(ql::changefeed::keyspec_t(
        std::move(range_keyspec),
        counted_t<base_table_t>(new artificial_table_t(this)),
        "<system table>"   /* I don't think this is ever used */
        ));
    guarantee(keyspec->table.has());

    *rows_out = make_counted<ql::vector_datum_stream_t>(
        bt, std::move(rows), std::move(keyspec));
    return true;
}

bool artificial_table_backend_t::read_all_rows_as_vector(
        UNUSED signal_t *interruptor,
        UNUSED std::vector<ql::datum_t> *rows_out,
        UNUSED admin_err_t *error_out) {
    crash("Oops, the default implementation of `artificial_table_backend_t::"
          "read_all_rows_as_vector()` was called. The `artificial_table_backend_t` "
          "subclass must override at least one of `read_all_rows_as_stream()` or "
          "`read_all_rows_as_vector()`. Also, the `artificial_table_backend_t` user "
          "shouldn't ever call `read_all_rows_as_vector()` directly.");
}
