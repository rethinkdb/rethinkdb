// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ADMIN_OP_EXC_HPP_
#define CLUSTERING_ADMINISTRATION_ADMIN_OP_EXC_HPP_

#include "utils.hpp"

#include "clustering/table_manager/table_meta_client.hpp"

struct admin_err_t {
    std::string msg;
    query_state_t query_state;
};

// These have to be macros because `rfail` and `rfail_datum` are macros.
#define REQL_RETHROW(X) do {                                            \
        rfail((X).query_state == query_state_t::FAILED                  \
              ? ql::base_exc_t::OP_FAILED                               \
              : ql::base_exc_t::OP_INDETERMINATE,                       \
              "%s", (X).msg.c_str());                                   \
    } while (0)
#define REQL_RETHROW_DATUM(X) do {                                      \
        rfail_datum((X).query_state == query_state_t::FAILED            \
                    ? ql::base_exc_t::OP_FAILED                         \
                    : ql::base_exc_t::OP_INDETERMINATE,                 \
                    "%s", (X).msg.c_str());                             \
    } while (0)


/* This is a generic class for errors that occur during administrative operations. It has
a string error message which is suitable for presenting to the user. */
class admin_op_exc_t : public std::runtime_error {
public:
    explicit admin_op_exc_t(admin_err_t err)
        : std::runtime_error(std::move(err.msg)),
          query_state(err.query_state) { }
    admin_op_exc_t(std::string msg, query_state_t _query_state)
        : std::runtime_error(std::move(msg)),
          query_state(_query_state) { }
    const query_state_t query_state;
};

/* `CATCH_NAME_ERRORS` and `CATCH_OP_ERRORS` are helper macros for catching the
exceptions thrown by the `table_meta_client_t` and producing consistent error messages.
They're designed to be used as follows:

    try {
        something_that_might_throw()
    } CATCH_NAME_ERRORS(db, name, error_out);

TODO: Probably these should re-throw `admin_op_exc_t` instead of setting `*error_out` and
returning `false`. */

#define CATCH_NAME_ERRORS(db, name, error_out)                                        \
    catch (const no_such_table_exc_t &) {                                             \
        *(error_out) = admin_err_t{                                                   \
            strprintf("Table `%s.%s` does not exist.", (db).c_str(), (name).c_str()), \
            query_state_t::FAILED};                                                   \
        return false;                                                                 \
    } catch (const ambiguous_table_exc_t &) {                                         \
        *(error_out) = admin_err_t{                                                   \
            strprintf("Table `%s.%s` is ambiguous; there are multiple "               \
                      "tables with that name.", (db).c_str(), (name).c_str()),        \
            query_state_t::FAILED};                                                   \
        return false;                                                                 \
    }

/* For read operations, `no_msg` and `maybe_msg` should both be `""`. For write
operations, `no_msg` should be a string literal stating that the operation was not
performed, and `maybe_msg` should be a string literal stating that the operation may or
may not have been performed. */
#define CATCH_OP_ERRORS(db, name, error_out, no_msg, maybe_msg)                       \
    catch (const failed_table_op_exc_t &) {                                           \
        *(error_out) = admin_err_t{                                                   \
            strprintf("The server(s) hosting table `%s.%s` are currently "            \
                      "unreachable. " no_msg " If you do not expect the server(s) "   \
                      "to recover, you can use `emergency_repair` to restore "        \
                      "availability of the table. "                                   \
                      "<http://rethinkdb.com/api/javascript/reconfigure/"             \
                      "#emergency-repair-mode>", (db).c_str(), (name).c_str()),       \
            query_state_t::FAILED};                                                   \
        return false;                                                                 \
    } catch (const maybe_failed_table_op_exc_t &) {                                   \
        *(error_out) = admin_err_t{                                                   \
            strprintf("We lost contact with the server(s) hosting table "             \
                      "`%s.%s`. " maybe_msg, (db).c_str(), (name).c_str()),           \
            query_state_t::INDETERMINATE};                                            \
        return false;                                                                 \
    }

#endif /* CLUSTERING_ADMINISTRATION_ADMIN_OP_EXC_HPP_ */

