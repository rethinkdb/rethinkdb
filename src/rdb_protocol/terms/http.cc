// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <stdint.h>

#include <string>
#include "debug.hpp"

#include "math.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/terms/terms.hpp"
#include "extproc/http_runner.hpp"

namespace ql {

class http_result_visitor_t : public boost::static_visitor<counted_t<val_t> > {
public:
    explicit http_result_visitor_t(const http_opts_t *_opts,
                                   const pb_rcheckable_t *_parent) :
          opts(_opts), parent(_parent) { }

    // This http resulted in an error
    counted_t<val_t> operator()(const std::string &err_val) const {
        rfail_target(parent, base_exc_t::GENERIC,
                     "Error in HTTP %s of `%s`: %s.",
                     http_method_to_str(opts->method).c_str(),
                     opts->url.c_str(), err_val.c_str());
    }

    // This http resulted in data
    counted_t<val_t> operator()(const counted_t<const datum_t> &datum) const {
        return make_counted<val_t>(datum, parent->backtrace());
    }

private:
    const http_opts_t *opts;
    const pb_rcheckable_t *parent;
};

class http_term_t : public op_term_t {
public:
    http_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        op_term_t(env, term, argspec_t(1),
                  optargspec_t({"data",
                                "timeout",
                                "method",
                                "params",
                                "header",
                                "attempts",
                                "redirects",
                                "verify",
                                "depaginate",
                                "auth",
                                "result_format" }))
    { }
private:
    virtual const char *name() const { return "http"; }

    // No HTTP term is considered deterministic
    virtual bool is_deterministic() const {
        return false;
    }

    virtual counted_t<val_t> eval_impl(scope_env_t *env,
                                       UNUSED eval_flags_t flags);

    // Functions to get optargs into the http_opts_t
    void get_optargs(scope_env_t *env, http_opts_t *opts_out);

    void get_bool_optarg(const std::string &optarg_name,
                         scope_env_t *env,
                         bool *bool_out);

    void get_redirects(scope_env_t *env,
                       uint32_t *redirects_out);

    void get_attempts(scope_env_t *env,
                      uint64_t *attempts_out);

    void get_result_format(scope_env_t *env,
                           http_result_format_t *result_format_out);

    void get_params(scope_env_t *env,
                    std::vector<std::pair<std::string, std::string> > *params_out);

    void get_data(scope_env_t *env,
                  std::string *data_out,
                  std::vector<std::pair<std::string, std::string> > *form_data_out,
                  std::vector<std::string> *header_out,
                  http_method_t method);

    void get_timeout_ms(scope_env_t *env,
                        uint64_t *timeout_ms_out);

    void get_header(scope_env_t *env,
                    std::vector<std::string> *header_out);

    void get_method(scope_env_t *env,
                    http_method_t *method_out);

    void get_auth(scope_env_t *env,
                  http_opts_t::http_auth_t *auth_out);

    // Helper functions, used in optarg parsing
    void verify_header_string(const std::string &str,
                              pb_rcheckable_t *header);

    std::string print_http_param(const counted_t<const datum_t> &datum,
                                 const char *val_name,
                                 const char *key_name,
                                 pb_rcheckable_t *val);

    std::string get_auth_item(const counted_t<const datum_t> &datum,
                              const std::string &name,
                              pb_rcheckable_t *auth);

    // Have a maximum timeout of 30 days
    static const uint64_t MAX_TIMEOUT_MS = 2592000000;
};

counted_t<val_t> http_term_t::eval_impl(scope_env_t *env,
                                        UNUSED eval_flags_t flags) {
    scoped_ptr_t<http_opts_t> opts(new http_opts_t());
    opts->url.assign(arg(env, 0)->as_str().to_std());
    opts->proxy.assign(env->env->reql_http_proxy);
    get_optargs(env, opts.get());

    http_result_t http_result;
    try {
        http_runner_t runner(env->env->extproc_pool);
        http_result = runner.http(opts.get(), env->env->interruptor);
    } catch (const http_worker_exc_t &ex) {
        http_result = std::string("crash in a worker process");
    } catch (const interrupted_exc_t &ex) {
        http_result = std::string("interrupted");
    } catch (const std::exception &ex) {
        http_result = std::string("encounted an exception - ") + ex.what();
    } catch (...) {
        http_result = std::string("encountered an unknown exception");
    }

    return boost::apply_visitor(http_result_visitor_t(opts.get(), this),
                                http_result);
}

void http_term_t::get_optargs(scope_env_t *env,
                              http_opts_t *opts_out) {
    get_auth(env, &opts_out->auth);
    get_method(env, &opts_out->method);

    // get_data must be called before get_header, as it may set the Content-Type header
    // in some cases, and the user should be able to override that behavior
    get_data(env, &opts_out->data, &opts_out->form_data,
             &opts_out->header, opts_out->method);

    get_result_format(env, &opts_out->result_format);
    get_params(env, &opts_out->url_params);
    get_header(env, &opts_out->header);
    get_timeout_ms(env, &opts_out->timeout_ms);
    get_attempts(env, &opts_out->attempts);
    get_redirects(env, &opts_out->max_redirects);
    // TODO: make depaginate a function, also a string to select a predefined style
    get_bool_optarg("depaginate", env, &opts_out->depaginate);
    get_bool_optarg("verify", env, &opts_out->verify);
}

// The `timeout` optarg specifies the number of seconds to wait before erroring
// out of the HTTP request.  This must be a NUMBER, but may be fractional.
void http_term_t::get_timeout_ms(scope_env_t *env,
                                 uint64_t *timeout_ms_out) {
    counted_t<val_t> timeout = optarg(env, "timeout");
    if (timeout.has()) {
        double tmp = timeout->as_num();
        tmp *= 1000;

        if (tmp < 0) {
            rfail_target(timeout.get(), base_exc_t::GENERIC,
                         "`timeout` may not be negative.");
        } else {
            *timeout_ms_out = clamp<uint64_t>(tmp, 0, MAX_TIMEOUT_MS);
        }
    }
}

// Don't allow header strings to include newlines
void http_term_t::verify_header_string(const std::string &str,
                                       pb_rcheckable_t *header) {
    if (str.find_first_of("\r\n") != std::string::npos) {
        rfail_target(header, base_exc_t::GENERIC,
                     "A `header` item contains newline characters.");
    }
}

// The `header` optarg allows the user to specify HTTP header values.
// The optarg must be an OBJECT or an ARRAY of STRINGs.
// As an OBJECT, each value must be a STRING or NULL, and each key/value pair
//  will be converted into a single header line with the format "key: value".
//  If the value is NULL, the header line will be "key:".
// As an ARRAY, each item must be a STRING, and each item will result
//  in exactly one header line, being copied directly over
// Header lines are not allowed to contain newlines.
void http_term_t::get_header(scope_env_t *env,
                             std::vector<std::string> *header_out) {
    counted_t<val_t> header = optarg(env, "header");
    if (header.has()) {
        counted_t<const datum_t> datum_header = header->as_datum();
        if (datum_header->get_type() == datum_t::R_OBJECT) {
            const std::map<std::string, counted_t<const datum_t> > &header_map =
                datum_header->as_object();
            for (auto it = header_map.begin(); it != header_map.end(); ++it) {
                std::string str;
                if (it->second->get_type() == datum_t::R_STR) {
                    str = strprintf("%s: %s", it->first.c_str(),
                                    it->second->as_str().c_str());
                } else if (it->second->get_type() != datum_t::R_NULL) {
                    rfail_target(header.get(), base_exc_t::GENERIC,
                        "Expected `header.%s` to be a STRING or NULL, but found %s.",
                        it->first.c_str(), it->second->get_type_name().c_str());
                }
                verify_header_string(str, header.get());
                header_out->push_back(str);
            }
        } else if (datum_header->get_type() == datum_t::R_ARRAY) {
            for (size_t i = 0; i < datum_header->size(); ++i) {
                counted_t<const datum_t> line = datum_header->get(i);
                if (line->get_type() != datum_t::R_STR) {
                    rfail_target(header.get(), base_exc_t::GENERIC,
                        "Expected `header[%zu]` to be a STRING, but found %s.",
                        i, line->get_type_name().c_str());
                }
                std::string str = line->as_str().to_std();
                verify_header_string(str, header.get());
                header_out->push_back(str);
            }
        } else {
            rfail_target(header.get(), base_exc_t::GENERIC,
                "Expected `header` to be an ARRAY or OBJECT, but found %s.",
                datum_header->get_type_name().c_str());
        }
    }
}

// The `method` optarg must be a STRING, and specify one of the following
// supported HTTP request methods: GET, HEAD, POST, PUT, PATCH, or DELETE.
void http_term_t::get_method(scope_env_t *env,
                             http_method_t *method_out) {
    counted_t<val_t> method = optarg(env, "method");
    if (method.has()) {
        std::string method_str = method->as_str().to_std();
        if (method_str == "GET") {
            *method_out = http_method_t::GET;
        } else if (method_str == "HEAD") {
            *method_out = http_method_t::HEAD;
        } else if (method_str == "POST") {
            *method_out = http_method_t::POST;
        } else if (method_str == "PUT") {
            *method_out = http_method_t::PUT;
        } else if (method_str == "PATCH") {
            *method_out = http_method_t::PATCH;
        } else if (method_str == "DELETE") {
            *method_out = http_method_t::DELETE;
        } else {
            rfail_target(method.get(), base_exc_t::GENERIC,
                         "`method` (%s) is not recognized (GET, HEAD, POST, PUT, "
                         "PATCH and DELETE are allowed).", method_str.c_str());
        }
    }
}

std::string http_term_t::get_auth_item(const counted_t<const datum_t> &datum,
                                       const std::string &name,
                                       pb_rcheckable_t *auth) {
    counted_t<const datum_t> item = datum->get(name, NOTHROW);
    if (!item.has()) {
        rfail_target(auth, base_exc_t::GENERIC,
                     "`auth.%s` not found in the auth object.", name.c_str());
    } else if (item->get_type() != datum_t::R_STR) {
        rfail_target(auth, base_exc_t::GENERIC,
                     "Expected `auth.%s` to be a STRING, but found %s.",
                     name.c_str(), item->get_type_name().c_str());
    }
    return item->as_str().to_std();
}

// The `auth` optarg takes an object consisting of the following fields:
//  type - STRING, the type of authentication to perform 'basic' or 'digest', defaults to 'basic'
//  user - STRING, the username to use
//  pass - STRING, the password to use
void http_term_t::get_auth(scope_env_t *env,
                           http_opts_t::http_auth_t *auth_out) {
    counted_t<val_t> auth = optarg(env, "auth");
    if (auth.has()) {
        counted_t<const datum_t> datum_auth = auth->as_datum();
        if (datum_auth->get_type() != datum_t::R_OBJECT) {
            rfail_target(auth.get(), base_exc_t::GENERIC,
                         "Expected `auth` to be an OBJECT, but found %s.",
                         datum_auth->get_type_name().c_str());
        }

        // Default to 'basic' if no type is specified
        std::string type;
        {
            counted_t<const datum_t> type_datum = datum_auth->get("type", NOTHROW);

            if (type_datum.has()) {
                if (type_datum->get_type() != datum_t::R_STR) {
                    rfail_target(auth.get(), base_exc_t::GENERIC,
                                 "Expected `auth.type` to be a STRING, but found %s.",
                                 datum_auth->get_type_name().c_str());
                }
                type.assign(type_datum->as_str().to_std());
            } else {
                type.assign("basic");
            }
        }

        std::string user = get_auth_item(datum_auth, "user", auth.get());
        std::string pass = get_auth_item(datum_auth, "pass", auth.get());

        if (type == "basic") {
            auth_out->make_basic_auth(std::move(user), std::move(pass));
        } else if (type == "digest") {
            auth_out->make_digest_auth(std::move(user), std::move(pass));
        } else {
            rfail_target(auth.get(), base_exc_t::GENERIC, "`auth.type` is not "
                         "recognized ('basic', and 'digest' are allowed).");
        }
    }
}

std::string http_term_t::print_http_param(const counted_t<const datum_t> &datum,
                                          const char *val_name,
                                          const char *key_name,
                                          pb_rcheckable_t *val) {
    if (datum->get_type() == datum_t::R_NUM) {
        return strprintf("%" PR_RECONSTRUCTABLE_DOUBLE,
                         datum->as_num());
    } else if (datum->get_type() == datum_t::R_STR) {
        return datum->as_str().to_std();
    } else if (datum->get_type() == datum_t::R_NULL) {
        return std::string();
    }

    rfail_target(val, base_exc_t::GENERIC,
                 "Expected `%s.%s` to be a NUMBER, STRING or NULL, but found %s.",
                 val_name, key_name, datum->get_type_name().c_str());
}

// The `data` optarg is used to pass in the data to be passed in the body of the
// request.  The sematics for this depend on the type of request being performed.
// This optarg is only allowed for PUT, POST, or PATCH requests.
// PUT, PATCH, and DELETE requests take any value and print it to JSON to pass to
//  the remote server (and will set Content-Type to application/json), unless the
//  value is a STRING, in which case it will be written directly to the request body.
// POST requests take either a STRING or OBJECT
//  POST with a STRING will pass the literal string in the request body
//  POST with an OBJECT must have NUMBER, STRING, or NULL values.  These will be
//   converted into a string of form-encoded key-value pairs of the format
//   "key=val&key=val" in the request body.
void http_term_t::get_data(scope_env_t *env,
        std::string *data_out,
        std::vector<std::pair<std::string, std::string> > *form_data_out,
        std::vector<std::string> *header_out,
        http_method_t method) {
    counted_t<val_t> data = optarg(env, "data");
    if (data.has()) {
        counted_t<const datum_t> datum_data = data->as_datum();
        if (method == http_method_t::PUT ||
            method == http_method_t::PATCH ||
            method == http_method_t::DELETE) {
            if (datum_data->get_type() == datum_t::R_STR) {
                data_out->assign(datum_data->as_str().to_std());
            } else {
                // Set the Content-Type to application/json - this may be overwritten
                // later by the 'header' optarg
                header_out->push_back("Content-Type: application/json");
                data_out->assign(datum_data->print());
            }
        } else if (method == http_method_t::POST) {
            if (datum_data->get_type() == datum_t::R_STR) {
                // Use the put data for this, as we assume the user does any
                // encoding they need when they pass a string
                data_out->assign(datum_data->as_str().to_std());
            } else if (datum_data->get_type() == datum_t::R_OBJECT) {
                const std::map<std::string, counted_t<const datum_t> > &form_map =
                    datum_data->as_object();
                for (auto it = form_map.begin(); it != form_map.end(); ++it) {
                    std::string val_str = print_http_param(it->second,
                                                           "data",
                                                           it->first.c_str(),
                                                           data.get());
                    form_data_out->push_back(std::make_pair(it->first, val_str));
                }
            } else {
                rfail_target(data.get(), base_exc_t::GENERIC,
                    "Expected `data` to be a STRING or OBJECT, but found %s.",
                    datum_data->get_type_name().c_str());
            }
        } else {
            rfail_target(this, base_exc_t::GENERIC,
                         "`data` should only be specified on a PUT, POST, PATCH, "
                         "or DELETE request.");
        }
    }
}

// The `params` optarg specifies parameters to append to the requested URL, in the
// format "?key=val&key=val". The optarg must be an OBJECT with NUMBER, STRING, or
// NULL values. A NULL value will result in "key=" with no value.
void http_term_t::get_params(scope_env_t *env,
        std::vector<std::pair<std::string, std::string> > *params_out) {
    counted_t<val_t> params = optarg(env, "params");
    if (params.has()) {
        counted_t<const datum_t> datum_params = params->as_datum();
        if (datum_params->get_type() == datum_t::R_OBJECT) {
            const std::map<std::string, counted_t<const datum_t> > &params_map =
                datum_params->as_object();
            for (auto it = params_map.begin(); it != params_map.end(); ++it) {
                std::string val_str = print_http_param(it->second,
                                                       "params",
                                                       it->first.c_str(),
                                                       params.get());
                params_out->push_back(std::make_pair(it->first, val_str));
            }

        } else {
            rfail_target(params.get(), base_exc_t::GENERIC,
                         "Expected `params` to be an OBJECT, but found %s.",
                         datum_params->get_type_name().c_str());
        }
    }
}

// The `result_format` optarg specifies how to interpret the HTTP result body from
// the server. This option must be a STRING, and one of `auto`, `json`, or `text`.
//  json - The result should be JSON and parsed into native datum_t objects
//  text - The result should be returned as a literal string
//  auto - The result will be parsed as JSON if the Content-Type is application/json,
//         or a string otherwise.
void http_term_t::get_result_format(scope_env_t *env,
                                    http_result_format_t *result_format_out) {
    counted_t<val_t> result_format = optarg(env, "result_format");
    if (result_format.has()) {
        std::string result_format_str = result_format->as_str().to_std();
        if (result_format_str == "auto") {
            *result_format_out = http_result_format_t::AUTO;
        } else if (result_format_str == "json") {
            *result_format_out = http_result_format_t::JSON;
        } else if (result_format_str == "text") {
            *result_format_out = http_result_format_t::TEXT;
        } else {
            rfail_target(result_format.get(), base_exc_t::GENERIC,
                         "`result_format` (%s) is not recognized, ('auto', 'json', "
                         "and 'text' are allowed).", result_format_str.c_str());
        }
    }
}

// The `attempts` optarg specifies the maximum number of times to attempt the
// request.  Reattempts will only be made when an HTTP error is returned that
// could feasibly be temporary.  This must be specified as an INTEGER >= 0.
void http_term_t::get_attempts(scope_env_t *env,
                               uint64_t *attempts_out) {
    counted_t<val_t> attempts = optarg(env, "attempts");
    if (attempts.has()) {
        *attempts_out = attempts->as_int<uint64_t>();
    }
}

// The `redirects` optarg specifies the maximum number of redirects to follow before
// erroring the query.  This must be passed as an INTEGER between 0 and 2^32 - 1.
void http_term_t::get_redirects(scope_env_t *env,
                                uint32_t *redirects_out) {
    counted_t<val_t> redirects = optarg(env, "redirects");
    if (redirects.has()) {
        *redirects_out = redirects->as_int<uint32_t>();
    }
}

// This is a generic function for parsing out a boolean optarg yet still providing a
// helpful message.  At the moment, it is only used for `depaginate` and `verify`.
void http_term_t::get_bool_optarg(const std::string &optarg_name,
                                  scope_env_t *env,
                                  bool *bool_out) {
    counted_t<val_t> option = optarg(env, optarg_name);
    if (option.has()) {
        *bool_out = option->as_bool();
    }
}

counted_t<term_t> make_http_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<http_term_t>(env, term);
}

}  // namespace ql
