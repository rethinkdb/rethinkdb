// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef EXTPROC_HTTP_RUNNER_HPP_
#define EXTPROC_HTTP_RUNNER_HPP_

#include <exception>
#include <string>
#include <vector>
#include <map>
#include <utility>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "containers/counted.hpp"
#include "rdb_protocol/datum.hpp"
#include "concurrency/signal.hpp"
#include "extproc/extproc_job.hpp"

// http calls result either in a DATUM return value or an error string
struct http_result_t {
    ql::datum_t header;
    ql::datum_t body;
    std::string error;
};

RDB_DECLARE_SERIALIZABLE(http_result_t);

class extproc_pool_t;
class http_runner_t;
class http_job_t;

enum class http_method_t {
    GET,
    HEAD,
    POST,
    PUT,
    PATCH,
    DELETE
};

enum class http_auth_type_t {
    NONE,
    BASIC,
    DIGEST
};

enum class http_result_format_t {
    AUTO,
    TEXT,
    JSON,
    JSONP,
    BINARY
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        http_method_t, int8_t,
        http_method_t::GET, http_method_t::DELETE);

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        http_auth_type_t, int8_t,
        http_auth_type_t::NONE, http_auth_type_t::DIGEST);

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        http_result_format_t, int8_t,
        http_result_format_t::AUTO, http_result_format_t::BINARY);

std::string http_method_to_str(http_method_t method);

struct http_opts_t {
    // Sets the default options
    http_opts_t();
    http_opts_t(http_opts_t &&) = default;

    struct http_auth_t {
        // No auth by default
        http_auth_t();
        http_auth_t(http_auth_t &&auth);

        void make_basic_auth(std::string &&user,
                             std::string &&pass);

        void make_digest_auth(std::string &&user,
                              std::string &&pass);

        http_auth_type_t type;
        std::string username;
        std::string password;
    };

    http_auth_t auth;

    http_method_t method;
    http_result_format_t result_format;

    std::string proxy;
    std::string url;
    ql::datum_t url_params;
    std::vector<std::string> header;

    // These will be used based on the method specified
    std::string data;
    std::map<std::string, std::string> form_data;

    // Limits on execution size
    ql::configured_limits_t limits;

    uint64_t timeout_ms;
    uint64_t attempts;
    uint32_t max_redirects;

    bool verify;
};

RDB_DECLARE_SERIALIZABLE(http_opts_t);
RDB_DECLARE_SERIALIZABLE(http_opts_t::http_auth_t);


// A handle to a running "HTTP fetcher" job.
class http_runner_t : public home_thread_mixin_t {
public:
    explicit http_runner_t(extproc_pool_t *_pool);

    void http(const http_opts_t &opts,
              http_result_t *res_out,
              signal_t *interruptor);

private:
    extproc_pool_t *pool;

    DISABLE_COPYING(http_runner_t);
};

#endif /* EXTPROC_HTTP_RUNNER_HPP_ */
