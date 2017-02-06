#include "rdb_protocol/artificial_table/backend.hpp"

#include <algorithm>

#include "clustering/administration/artificial_reql_cluster_interface.hpp"
#include "rdb_protocol/artificial_table/artificial_table.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/datum_stream/vector.hpp"

const uuid_u artificial_table_backend_t::base_table_id =
    str_to_uuid("0eabef01-6deb-4069-9a2d-448db057ab1e");

artificial_table_backend_t::artificial_table_backend_t(
        name_string_t const &table_name,
        rdb_context_t *rdb_context)
    : m_table_name(table_name),
      m_table_id(uuid_u::from_hash(base_table_id, table_name.str())),
      m_rdb_context(rdb_context) {
}

artificial_table_backend_t::~artificial_table_backend_t() {
}

uuid_u const &artificial_table_backend_t::get_table_id() const {
    return m_table_id;
}

bool artificial_table_backend_t::read_all_rows_filtered(
        auth::user_context_t const &user_context,
        const ql::datumspec_t &datumspec,
        sorting_t sorting,
        signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        admin_err_t *error_out) {

    /* Fetch the rows from the backend */
    std::vector<ql::datum_t> rows;
    if (!read_all_rows_as_vector(user_context, interruptor, &rows, error_out)) {
        return false;
    }

    std::string primary_key = get_primary_key_name();

    /* Apply range filter */
    if (!datumspec.is_universe()) {
        std::vector<ql::datum_t> filter_rows;
        for (const auto &row : rows) {
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

    *rows_out = std::move(rows);
    return true;
}

bool artificial_table_backend_t::read_all_rows_filtered_as_stream(
        auth::user_context_t const &user_context,
        ql::backtrace_id_t bt,
        const ql::datumspec_t &datumspec,
        sorting_t sorting,
        signal_t *interruptor,
        counted_t<ql::datum_stream_t> *rows_out,
        admin_err_t *error_out) {
    std::vector<ql::datum_t> rows;
    if (!read_all_rows_filtered(user_context, datumspec, sorting, interruptor,
                                &rows, error_out)) {
        return false;
    }

    ql::changefeed::keyspec_t::range_t range_keyspec = {
        std::vector<ql::transform_variant_t>(),
        r_nullopt,
        sorting,
        datumspec,
        r_nullopt
    };
    optional<ql::changefeed::keyspec_t> keyspec(ql::changefeed::keyspec_t(
        std::move(range_keyspec),
        counted_t<base_table_t>(
            new artificial_table_t(
                m_rdb_context, artificial_reql_cluster_interface_t::database_id, this)),
        m_table_name.str()));
    guarantee(keyspec->table.has());

    *rows_out = make_counted<ql::vector_datum_stream_t>(
        bt, std::move(rows), std::move(keyspec));
    return true;
}

