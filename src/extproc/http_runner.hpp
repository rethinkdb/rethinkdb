// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef EXTPROC_HTTP_RUNNER_HPP_
#define EXTPROC_HTTP_RUNNER_HPP_

#include <exception>
#include <string>
#include <vector>
#include <utility>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "containers/counted.hpp"
#include "rdb_protocol/datum.hpp"
#include "concurrency/signal.hpp"

// http calls result either in a DATUM return value or an error string
typedef boost::variant<counted_t<const ql::datum_t>, std::string> http_result_t;

class extproc_pool_t;
class http_runner_t;
class http_job_t;

class http_worker_exc_t : public std::exception {
public:
    explicit http_worker_exc_t(const std::string& data) : info(data) { }
    ~http_worker_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

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
    JSON
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        http_method_t, int8_t,
        http_method_t::GET, http_method_t::DELETE);

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        http_auth_type_t, int8_t,
        http_auth_type_t::NONE, http_auth_type_t::DIGEST);

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        http_result_format_t, int8_t,
        http_result_format_t::AUTO, http_result_format_t::JSON);

std::string http_method_to_str(http_method_t method);

struct http_opts_t {
    // Sets the default options
    http_opts_t();

    struct http_auth_t {
        // No auth by default
        http_auth_t();

        void make_basic_auth(std::string &&user,
                             std::string &&pass);

        void make_digest_auth(std::string &&user,
                              std::string &&pass);

        http_auth_type_t type;
        std::string username;
        std::string password;

        RDB_DECLARE_ME_SERIALIZABLE;
    } auth;

    http_method_t method;
    http_result_format_t result_format;

    std::string proxy;
    std::string url;
    std::vector<std::pair<std::string, std::string> > url_params;
    std::vector<std::string> header;

    // These will be used based on the method specified
    std::string data;
    std::vector<std::pair<std::string, std::string> > form_data;

    uint64_t timeout_ms;
    uint64_t attempts;
    uint32_t max_redirects;

    bool depaginate;
    bool verify;

    RDB_DECLARE_ME_SERIALIZABLE;
};

// A handle to a running "javascript evaluator" job.
class http_runner_t : public home_thread_mixin_t {
public:
    explicit http_runner_t(extproc_pool_t *_pool);

    http_result_t http(const http_opts_t *opts,
                       signal_t *interruptor);

private:
    extproc_pool_t *pool;

    DISABLE_COPYING(http_runner_t);
};

#endif /* EXTPROC_HTTP_RUNNER_HPP_ */
