// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "extproc/http_runner.hpp"

#include "extproc/http_job.hpp"
#include "containers/archive/stl_types.hpp"
#include "arch/timing.hpp"

RDB_IMPL_ME_SERIALIZABLE_3_SINCE_v1_13(http_result_t, empty_ok(header), empty_ok(body), error);
RDB_IMPL_SERIALIZABLE_3_SINCE_v1_13(http_opts_t::http_auth_t, type, username, password);
RDB_IMPL_ME_SERIALIZABLE_14(http_opts_t, auth, method, result_format, url,
                            proxy, empty_ok(url_params), header, data,
                            form_data, limits, timeout_ms, attempts,
                            max_redirects, verify);
INSTANTIATE_SERIALIZABLE_SELF_FOR_CLUSTER(http_opts_t);

std::string http_method_to_str(http_method_t method) {
    switch(method) {
    case http_method_t::GET:    return std::string("GET");
    case http_method_t::HEAD:   return std::string("HEAD");
    case http_method_t::POST:   return std::string("POST");
    case http_method_t::PUT:    return std::string("PUT");
    case http_method_t::PATCH:  return std::string("PATCH");
    case http_method_t::DELETE: return std::string("DELETE");
    default:                    return std::string("UNKNOWN");
    }
}

http_opts_t::http_opts_t() :
    auth(),
    method(http_method_t::GET),
    result_format(http_result_format_t::AUTO),
    proxy(),
    url(),
    url_params(),
    header(),
    data(),
    form_data(),
    limits(),
    timeout_ms(30000),
    attempts(5),
    max_redirects(1),
    verify(true) { }

http_opts_t::http_auth_t::http_auth_t() :
    type(http_auth_type_t::NONE),
    username(),
    password() { }

http_opts_t::http_auth_t::http_auth_t(http_auth_t &&other) :
    type(other.type),
    username(std::move(other.username)),
    password(std::move(other.password)) { }

void http_opts_t::http_auth_t::make_basic_auth(std::string &&user,
                                               std::string &&pass) {
    type = http_auth_type_t::BASIC;
    username.assign(std::move(user));
    password.assign(std::move(pass));
}

void http_opts_t::http_auth_t::make_digest_auth(std::string &&user,
                                                std::string &&pass) {
    type = http_auth_type_t::DIGEST;
    username.assign(std::move(user));
    password.assign(std::move(pass));
}

http_runner_t::http_runner_t(extproc_pool_t *_pool) :
    pool(_pool) { }

void http_runner_t::http(const http_opts_t &opts,
                         http_result_t *res_out,
                         signal_t *interruptor) {
    signal_timer_t timeout;
    wait_any_t combined_interruptor(interruptor, &timeout);
    http_job_t job(pool, &combined_interruptor);

    assert_thread();
    timeout.start(opts.timeout_ms);

    try {
        job.http(opts, res_out);
    } catch (const interrupted_exc_t &ex) {
        if (!timeout.is_pulsed()) {
            throw;
        }
        res_out->error =
            strprintf("timed out after %" PRIu64 ".%03" PRIu64 " seconds",
                      opts.timeout_ms / 1000, opts.timeout_ms % 1000);
    } catch (...) {
        // This will mark the worker as errored so we don't try to re-sync with it
        //  on the next line (since we're in a catch statement, we aren't allowed)
        job.worker_error();
        throw;
    }
}
