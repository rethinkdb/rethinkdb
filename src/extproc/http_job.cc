// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "extproc/http_job.hpp"

#include <ctype.h>
#include <re2/re2.h>

#ifdef _WIN32
#define CURL_STATICLIB
#endif
#include <curl/curl.h>

#include <limits>

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "extproc/extproc_job.hpp"
#include "http/http_parser.hpp"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rdb_protocol/env.hpp"

#define RETHINKDB_USER_AGENT (SOFTWARE_NAME_STRING "/" RETHINKDB_VERSION)

void parse_header(const std::string &header,
                  http_result_t *res_out);

void save_cookies(CURL *curl_handle,
                  http_result_t *res_out);

enum class attach_json_to_error_t { YES, NO };
void json_to_datum(const std::string &json,
                   const ql::configured_limits_t &limits,
                   reql_version_t reql_version,
                   attach_json_to_error_t attach_json,
                   http_result_t *res_out);

void jsonp_to_datum(const std::string &jsonp,
                    const ql::configured_limits_t &limits,
                    reql_version_t reql_version,
                    attach_json_to_error_t attach_json,
                    http_result_t *res_out);

void perform_http(http_opts_t *opts,
                  http_result_t *res_out);

class curl_exc_t : public std::exception {
public:
    explicit curl_exc_t(std::string err_msg) :
        error_string(std::move(err_msg)) { }
    ~curl_exc_t() throw () { }
    const char *what() const throw () {
        return error_string.c_str();
    }
    const std::string error_string;
};

class scoped_curl_handle_t {
public:
    scoped_curl_handle_t() :
        curl_handle(curl_easy_init()) {
    }

    ~scoped_curl_handle_t() {
        curl_easy_cleanup(curl_handle);
    }

    CURL *get() {
        return curl_handle;
    }

private:
    CURL *curl_handle;
};

// Used for adding headers, which cannot be freed until after the request is done
class scoped_curl_slist_t {
public:
    scoped_curl_slist_t() :
        slist(nullptr) { }

    ~scoped_curl_slist_t() {
        if (slist != nullptr) {
            curl_slist_free_all(slist);
        }
    }

    curl_slist *get() {
        return slist;
    }

    void add(const std::string &str) {
        slist = curl_slist_append(slist, str.c_str());
        if (slist == nullptr) {
            throw curl_exc_t("appending header, allocation failure");
        }
    }

private:
    struct curl_slist *slist;
};

class curl_data_t {
public:
    curl_data_t() : send_data_offset(0) { }

    static size_t write_body(char *ptr, size_t size, size_t nmemb, void* instance);
    static size_t write_header(char *ptr, size_t size, size_t nmemb, void* instance);
    static size_t read(char *ptr, size_t size, size_t nmemb, void* instance);

    void set_send_data(std::string &&_send_data) {
        send_data.assign(std::move(_send_data));
    }

    std::string steal_header_data() {
        return std::move(header_data);
    }

    std::string steal_body_data() {
        return std::move(body_data);
    }

    scoped_curl_slist_t header;

private:
    // These are called for getting data in the response from the server
    size_t write_body_internal(char *ptr, size_t size);
    size_t write_header_internal(char *ptr, size_t size);

    // This is called for writing data to the request when sending
    size_t read_internal(char *ptr, size_t size);

    size_t send_data_offset;
    std::string send_data;
    std::string body_data;
    std::string header_data;
};

size_t multiply_with_overflow_check(size_t a, size_t b, const std::string &info) {
    if (b == 0 || a > std::numeric_limits<size_t>::max() / b) {
        throw curl_exc_t(info + " data size too large");
    }
    return a * b;
}

size_t curl_data_t::write_header(char *ptr, size_t size, size_t nmemb, void* instance) {
    curl_data_t *self = static_cast<curl_data_t *>(instance);
    return self->write_header_internal(ptr,
                                       multiply_with_overflow_check(size, nmemb,
                                                                    "write header"));
}

size_t curl_data_t::write_body(char *ptr, size_t size, size_t nmemb, void* instance) {
    curl_data_t *self = static_cast<curl_data_t *>(instance);
    return self->write_body_internal(ptr,
                                     multiply_with_overflow_check(size, nmemb,
                                                                  "write body"));
}

size_t curl_data_t::read(char *ptr, size_t size, size_t nmemb, void* instance) {
    curl_data_t *self = static_cast<curl_data_t *>(instance);
    return self->read_internal(ptr, multiply_with_overflow_check(size, nmemb, "read"));
}

size_t curl_data_t::write_header_internal(char *ptr, size_t size) {
    header_data.append(ptr, size);
    return size;
}

size_t curl_data_t::write_body_internal(char *ptr, size_t size) {
    body_data.append(ptr, size);
    return size;
}

size_t curl_data_t::read_internal(char *ptr, size_t size) {
    size_t bytes_to_copy = std::min( { size,
                                       send_data.size() - send_data_offset } );
    memcpy(ptr, send_data.data() + send_data_offset, bytes_to_copy);
    send_data_offset += bytes_to_copy;
    return bytes_to_copy;
}

// The job_t runs in the context of the main rethinkdb process
http_job_t::http_job_t(extproc_pool_t *pool, signal_t *interruptor) :
    extproc_job(pool, &worker_fn, interruptor) { }

void http_job_t::http(const http_opts_t &opts,
                      http_result_t *res_out) {
    write_message_t msg;
    serialize<cluster_version_t::LATEST_OVERALL>(&msg, opts);
    {
        int res = send_write_message(extproc_job.write_stream(), &msg);
        if (res != 0) {
            throw extproc_worker_exc_t("failed to send data to the worker");
        }
    }

    archive_result_t res
        = deserialize<cluster_version_t::LATEST_OVERALL>(extproc_job.read_stream(),
                                                         res_out);
    if (bad(res)) {
        throw extproc_worker_exc_t(strprintf("failed to deserialize result from worker "
                                             "(%s)", archive_result_as_str(res)));
    }
}

void http_job_t::worker_error() {
    extproc_job.worker_error();
}

bool http_job_t::worker_fn(read_stream_t *stream_in, write_stream_t *stream_out) {
    static bool curl_initialized(false);
    http_opts_t opts;
    {
        archive_result_t res
            = deserialize<cluster_version_t::LATEST_OVERALL>(stream_in, &opts);
        if (bad(res)) { return false; }
    }

    http_result_t result;
    result.header = ql::datum_t::null();
    result.body = ql::datum_t::null();

    CURLcode curl_res = CURLE_OK;
    if (!curl_initialized) {
        curl_res = curl_global_init(CURL_GLOBAL_SSL);
        curl_initialized = true;
    }

    if (curl_res == CURLE_OK) {
        try {
            perform_http(&opts, &result);
        } catch (const std::exception &ex) {
            result.error.assign(ex.what());
        } catch (...) {
            result.error.assign("unknown error");
        }
    } else {
        result.error.assign("global initialization");
        curl_initialized = false;
    }

    write_message_t msg;
    serialize<cluster_version_t::LATEST_OVERALL>(&msg, result);
    int res = send_write_message(stream_out, &msg);
    if (res != 0) { return false; }

    return true;
}

std::string exc_encode(CURL *curl_handle, const std::string &str) {
    std::string res;

    if (str.size() != 0) {
        char *curl_res = curl_easy_escape(curl_handle, str.data(), str.size());
        if (curl_res == nullptr) {
            throw curl_exc_t("encode string");
        }
        res.assign(curl_res);
        curl_free(curl_res);
    }

    return res;
}

// We may get multiple headers due to authentication or redirection.  Filter out all but
// the last response's header, which is all we care about.
void truncate_header_data(std::string *header_data) {
    size_t last_crlf = header_data->rfind("\r\n\r\n");

    if (last_crlf != std::string::npos || last_crlf > 0) {
        size_t second_to_last_crlf = header_data->rfind("\r\n\r\n", last_crlf - 1);
        if (second_to_last_crlf != std::string::npos) {
            header_data->erase(0, second_to_last_crlf + 4);
        }
    }
}

template <class T>
void exc_setopt(CURL *curl_handle, CURLoption opt, T val, const char *info) {
    CURLcode curl_res = curl_easy_setopt(curl_handle, opt, val);
    if (curl_res != CURLE_OK) {
        throw curl_exc_t(strprintf("set option %s, '%s'",
                                   info, curl_easy_strerror(curl_res)));
    }
}

void transfer_auth_opt(const http_opts_t::http_auth_t &auth, CURL *curl_handle) {
    if (auth.type != http_auth_type_t::NONE) {
        long curl_auth_type; // NOLINT(runtime/int)
        switch (auth.type) {
        case http_auth_type_t::BASIC:
            curl_auth_type = CURLAUTH_BASIC;
            break;
        case http_auth_type_t::DIGEST:
            curl_auth_type = CURLAUTH_DIGEST;
            break;
        case http_auth_type_t::NONE:
        default:
            unreachable();
        }
        exc_setopt(curl_handle, CURLOPT_HTTPAUTH, curl_auth_type, "HTTP AUTH TYPE");
        exc_setopt(curl_handle, CURLOPT_USERNAME,
                   auth.username.c_str(), "HTTP AUTH USERNAME");
        exc_setopt(curl_handle, CURLOPT_PASSWORD,
                   auth.password.c_str(), "HTTP AUTH PASSWORD");
    }
}

void add_read_callback(CURL *curl_handle,
                       std::string &&data,
                       curl_data_t *curl_data) {
    curl_off_t size = data.size();
    if (size != 0) {
        curl_data->set_send_data(std::move(data));
        exc_setopt(curl_handle, CURLOPT_READFUNCTION,
                   &curl_data_t::read, "READ FUNCTION");
        exc_setopt(curl_handle, CURLOPT_READDATA, curl_data, "READ DATA");
        exc_setopt(curl_handle, CURLOPT_INFILESIZE_LARGE, size, "DATA SIZE");
    }
}

void url_encode_kv(CURL *curl_handle,
                   const std::string &key,
                   const std::string &val,
                   std::string *str_out) {
    str_out->append(exc_encode(curl_handle, key));
    str_out->push_back('=');
    str_out->append(exc_encode(curl_handle, val));
}

std::string url_encode_fields(CURL *curl_handle,
        const std::map<std::string, std::string> &fields) {
    std::string data;
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        if (it != fields.begin()) {
            data.push_back('&');
        }
        url_encode_kv(curl_handle, it->first, it->second, &data);
    }
    return data;
}

std::string url_encode_fields(CURL *curl_handle,
                              const ql::datum_t &fields) {
    // Convert to a common format
    if (!fields.has()) {
        return std::string();
    }

    std::map<std::string, std::string> translated_fields;

    for (size_t field_idx = 0; field_idx < fields.obj_size(); ++field_idx) {
        auto pair = fields.get_pair(field_idx);
        std::string val;
        if (pair.second.get_type() == ql::datum_t::R_NUM) {
            val = strprintf("%" PR_RECONSTRUCTABLE_DOUBLE,
                            pair.second.as_num());
        } else if (pair.second.get_type() == ql::datum_t::R_STR) {
            val = pair.second.as_str().to_std();
        } else if (pair.second.get_type() != ql::datum_t::R_NULL) {
            // This shouldn't happen because we check this in the main process anyway
            throw curl_exc_t(strprintf("expected `params.%s` to be a NUMBER, STRING, "
                                       "or nullptr, but found %s",
                                       pair.first.to_std().c_str(),
                                       pair.second.get_type_name().c_str()));
        }
        translated_fields[pair.first.to_std()] = val;
    }

    return url_encode_fields(curl_handle, translated_fields);
}

void transfer_method_opt(http_opts_t *opts,
                         CURL *curl_handle,
                         curl_data_t *curl_data) {
    // Opts will have either data or form_data set -
    //  - form_data is only used for post requests, and will result in a string
    //    of form-encoded pairs in the request
    //  - data is used in put, patch, and post requests, and will result in the
    //    given string being directly put into the body of the request
    switch (opts->method) {
        case http_method_t::GET:
            exc_setopt(curl_handle, CURLOPT_HTTPGET, 1, "HTTP GET");
            break;
        case http_method_t::PATCH:
            exc_setopt(curl_handle, CURLOPT_UPLOAD, 1, "HTTP PUT");
            exc_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PATCH", "HTTP PATCH");
            add_read_callback(curl_handle, std::move(opts->data), curl_data);
            break;
        case http_method_t::PUT:
            exc_setopt(curl_handle, CURLOPT_UPLOAD, 1, "HTTP PUT");
            add_read_callback(curl_handle, std::move(opts->data), curl_data);
            break;
        case http_method_t::POST:
            if (!opts->form_data.empty()) {
                // This is URL-encoding the form data, which isn't *exactly* the same as
                // x-www-url-formencoded, but it should be compatible
                opts->data = url_encode_fields(curl_handle, opts->form_data);
            }

            exc_setopt(curl_handle, CURLOPT_POST, 1, "HTTP POST");
            exc_setopt(curl_handle, CURLOPT_POSTFIELDS,
                       opts->data.data(), "HTTP POST DATA");
            exc_setopt(curl_handle, CURLOPT_POSTFIELDSIZE,
                       opts->data.size(), "HTTP POST DATA SIZE");
            break;
        case http_method_t::HEAD:
            exc_setopt(curl_handle, CURLOPT_NOBODY, 1, "HTTP HEAD");
            break;
        case http_method_t::DELETE:
            exc_setopt(curl_handle, CURLOPT_UPLOAD, 1, "HTTP PUT");
            exc_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE", "HTTP DELETE");
            add_read_callback(curl_handle, std::move(opts->data), curl_data);
            break;
        default:
            unreachable();
    }
}

void transfer_url_opt(const std::string &url,
        const ql::datum_t &url_params,
        CURL *curl_handle) {
    std::string full_url = url;
    std::string params = url_encode_fields(curl_handle, url_params);

    // Handle cases where the url already has parameters, or is missing a '/' at the end
    // We need to prepare the url for appending url-encoded parameters
    // Examples:
    //  example.org => example.org/?
    //  example.org?id=0 => example.org?id=0& - This was already malformed, but w/e
    //  example.org/page => example.org/page?
    //  example.org/page?id=0 => example.org/page?id=0&
    //  example.org/page?id=0/page => example.org/page?id=0/page?
    if (params.size() > 0) {
        size_t params_pos = url.rfind('?');
        size_t slash_pos = url.rfind('/');

        if (slash_pos == std::string::npos &&
            params_pos == std::string::npos) {
            full_url.push_back('/');
        }

        if (params_pos == std::string::npos ||
            (slash_pos != std::string::npos &&
             slash_pos > params_pos)) {
            full_url.push_back('?');
        } else if (slash_pos != std::string::npos &&
                   params_pos != std::string::npos &&
                   slash_pos < params_pos) {
            full_url.push_back('&');
        }

        full_url.append(params);
    }

    exc_setopt(curl_handle, CURLOPT_URL, full_url.c_str(), "URL");
}

void transfer_header_opt(const std::vector<std::string> &header,
                         CURL *curl_handle,
                         curl_data_t *curl_data) {
    for (auto it = header.begin(); it != header.end(); ++it) {
        curl_data->header.add(*it);
    }

    if (curl_data->header.get() != nullptr) {
        exc_setopt(curl_handle, CURLOPT_HTTPHEADER, curl_data->header.get(), "HEADER");
    }
}

void transfer_redirect_opt(uint32_t max_redirects, CURL *curl_handle) {
    long val = (max_redirects > 0) ? 1 : 0; // NOLINT(runtime/int)
    exc_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, val, "ALLOW REDIRECT");
    val = max_redirects;
    exc_setopt(curl_handle, CURLOPT_MAXREDIRS, val, "MAX REDIRECTS");
    // maybe we should set CURLOPT_POSTREDIR - libcurl will, by default,
    // change POST requests to GET requests if redirected
}

void transfer_verify_opt(bool verify, CURL *curl_handle) {
    long val = verify ? 1 : 0; // NOLINT(runtime/int)
    exc_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, val, "SSL VERIFY PEER");
    val = verify ? 2 : 0;
    exc_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, val, "SSL VERIFY HOST");
}

void transfer_cookies(const std::vector<std::string> &cookies, CURL *curl_handle) {
    for (auto const &cookie : cookies) {
        exc_setopt(curl_handle, CURLOPT_COOKIELIST, cookie.c_str(), "COOKIELIST");
    }
}

void transfer_opts(http_opts_t *opts,
                   CURL *curl_handle,
                   curl_data_t *curl_data) {
    transfer_auth_opt(opts->auth, curl_handle);
    transfer_url_opt(opts->url, opts->url_params, curl_handle);
    transfer_redirect_opt(opts->max_redirects, curl_handle);
    transfer_verify_opt(opts->verify, curl_handle);

    transfer_header_opt(opts->header, curl_handle, curl_data);

    // Set method last as it may override some options libcurl automatically sets
    transfer_method_opt(opts, curl_handle, curl_data);

    // Copy any saved cookies from a previous request (e.g. in the case of depagination)
    transfer_cookies(opts->cookies, curl_handle);
}

void set_default_opts(CURL *curl_handle,
                      const std::string &proxy,
                      const curl_data_t &curl_data) {
    exc_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
               &curl_data_t::write_body, "WRITE FUNCTION");
    exc_setopt(curl_handle, CURLOPT_WRITEDATA, &curl_data, "WRITE DATA");

    exc_setopt(curl_handle, CURLOPT_HEADERFUNCTION,
               &curl_data_t::write_header, "HEADER FUNCTION");
    exc_setopt(curl_handle, CURLOPT_HEADERDATA, &curl_data, "HEADER DATA");

    // Only allow http protocol
    exc_setopt(curl_handle, CURLOPT_PROTOCOLS,
               CURLPROTO_HTTP | CURLPROTO_HTTPS, "PROTOCOLS");

    exc_setopt(curl_handle, CURLOPT_USERAGENT, RETHINKDB_USER_AGENT, "USER AGENT");

    exc_setopt(curl_handle, CURLOPT_ENCODING, "deflate;q=1, gzip;q=0.5", "PROTOCOLS");

    exc_setopt(curl_handle, CURLOPT_NOSIGNAL, 1, "NOSIGNAL");

    // Enable cookies - needed for multiple requests like redirects or digest auth
    exc_setopt(curl_handle, CURLOPT_COOKIEFILE, "", "COOKIEFILE");

    // Use the proxy set when launched
    if (!proxy.empty()) {
        exc_setopt(curl_handle, CURLOPT_PROXY, proxy.c_str(), "PROXY");
    }
}

// TODO: implement streaming API support
void perform_http(http_opts_t *opts, http_result_t *res_out) {
    scoped_curl_handle_t curl_handle;
    curl_data_t curl_data;

    if (curl_handle.get() == nullptr) {
        res_out->error.assign("initialization");
        return;
    }

    set_default_opts(curl_handle.get(), opts->proxy, curl_data);
    transfer_opts(opts, curl_handle.get(), &curl_data);

    CURLcode curl_res = CURLE_OK;
    long response_code = 0; // NOLINT(runtime/int)
    for (uint64_t attempts = 0; attempts < opts->attempts; ++attempts) {
        // Do the HTTP operation, then check for errors
        curl_res = curl_easy_perform(curl_handle.get());

        if (curl_res == CURLE_SEND_ERROR ||
            curl_res == CURLE_RECV_ERROR ||
            curl_res == CURLE_COULDNT_CONNECT) {
            // Could be a temporary error, try again
            continue;
        } else if (curl_res != CURLE_OK) {
            res_out->error.assign(curl_easy_strerror(curl_res));
            return;
        }

        curl_res = curl_easy_getinfo(curl_handle.get(),
                                     CURLINFO_RESPONSE_CODE,
                                     &response_code);

        // Break on success, retry on temporary error
        if (curl_res != CURLE_OK ||
            // Error codes that may be resolved by retrying
            (response_code != 408 &&
             response_code != 500 &&
             response_code != 502 &&
             response_code != 503 &&
             response_code != 504)) {
            break;
        }
    }

    std::string body_data(curl_data.steal_body_data());
    std::string header_data(curl_data.steal_header_data());
    truncate_header_data(&header_data);

    if (opts->attempts == 0) {
        res_out->error.assign("could not perform, no attempts allowed");
    } else if (curl_res == CURLE_SEND_ERROR) {
        res_out->error.assign("error when sending data");
    } else if (curl_res == CURLE_RECV_ERROR) {
        res_out->error.assign("error when receiving data");
    } else if (curl_res == CURLE_COULDNT_CONNECT) {
        res_out->error.assign("could not connect to server");
    } else if (curl_res != CURLE_OK) {
        res_out->error = strprintf("reading response code, '%s'",
                                   curl_easy_strerror(curl_res));
    } else if (response_code < 200 || response_code >= 300) {
        if (!header_data.empty()) {
            parse_header(header_data, res_out);
        }
        if (!body_data.empty()) {
            res_out->body = ql::datum_t(datum_string_t(body_data));
        }
        res_out->error = strprintf("status code %ld", response_code);
    } else {
        parse_header(header_data, res_out);
        save_cookies(curl_handle.get(), res_out);

        // If this was a HEAD request, we should not be handling data, just return R_NULL
        // so the user knows the request succeeded
        if (opts->method == http_method_t::HEAD) {
            res_out->body = ql::datum_t::null();
            return;
        }

        rassert(res_out->error.empty());

        switch (opts->result_format) {
        case http_result_format_t::AUTO:
            {
                std::string content_type;
                char *content_type_buffer = nullptr;
                curl_easy_getinfo(curl_handle.get(),
                                  CURLINFO_CONTENT_TYPE,
                                  &content_type_buffer);

                if (content_type_buffer != nullptr) {
                    content_type.assign(content_type_buffer);
                }

                for (size_t i = 0; i < content_type.length(); ++i) {
                    content_type[i] = tolower(content_type[i]);
                }

                if (content_type.find("application/json") == 0) {
                    json_to_datum(body_data, opts->limits, opts->version,
                                  attach_json_to_error_t::YES, res_out);
                } else if (content_type.find("text/javascript") == 0 ||
                           content_type.find("application/json-p") == 0 ||
                           content_type.find("text/json-p") == 0) {
                    // Try to parse the result as JSON, then as JSONP, then plaintext
                    // Do not use move semantics here, as we retry on errors
                    json_to_datum(body_data, opts->limits, opts->version,
                                  attach_json_to_error_t::NO, res_out);
                    if (!res_out->error.empty()) {
                        res_out->error.clear();
                        jsonp_to_datum(body_data, opts->limits, opts->version,
                                       attach_json_to_error_t::NO, res_out);
                        if (!res_out->error.empty()) {
                            res_out->error.clear();
                            res_out->body =
                                ql::datum_t(datum_string_t(body_data));
                        }
                    }
                } else if (content_type.find("audio/") == 0 ||
                           content_type.find("image/") == 0 ||
                           content_type.find("video/") == 0 ||
                           content_type.find("application/octet-stream") == 0) {
                    res_out->body = ql::datum_t::binary(datum_string_t(body_data));

                } else {
                    res_out->body =
                        ql::datum_t(datum_string_t(body_data));
                }
            }
            break;
        case http_result_format_t::JSON:
            json_to_datum(body_data, opts->limits, opts->version,
                          attach_json_to_error_t::YES, res_out);
            break;
        case http_result_format_t::JSONP:
            jsonp_to_datum(body_data, opts->limits, opts->version,
                           attach_json_to_error_t::YES, res_out);
            break;
        case http_result_format_t::TEXT:
            res_out->body = ql::datum_t(datum_string_t(body_data));
            break;
        case http_result_format_t::BINARY:
            res_out->body = ql::datum_t::binary(datum_string_t(body_data));
            break;
        default:
            unreachable();
        }
    }
}

class header_parser_singleton_t {
public:
    static ql::datum_t parse(const std::string &header);

private:
    static void initialize();
    static header_parser_singleton_t *instance;

    header_parser_singleton_t();

    // Callbacks from http-parser
    static int on_headers_complete(UNUSED http_parser *parser);
    static int on_header_field(http_parser *parser, const char *field, size_t length);
    static int on_header_value(http_parser *parser, const char *value, size_t length);

    // Helper function to special-case 'Link' header parsing
    void add_link_header(const std::string &line);

    http_parser_settings settings;
    http_parser parser;

    RE2 link_parser;
    std::map<datum_string_t, ql::datum_t> header_fields;
    std::map<datum_string_t, ql::datum_t> link_headers;
    std::string current_field;
};
header_parser_singleton_t *header_parser_singleton_t::instance = nullptr;

// The link_parser regular expression is used to parse 'Link' headers from HTTP
// responses (see RFC 5988 - http://tools.ietf.org/html/rfc5988).  We are interested
// in parsing out one or two strings for each link.  Examples:
//
// Link: <http://example.org>; rel="next"
//  - we want the strings 'http://example.org' and 'rel="next"'
// Link: <http://example.org>; title="Arbitrary String", <http://example.org>
//  - this is two links on the same header line, but should be parsed separately
//  - for the first, we want 'http://example.org' and 'title="Arbitrary String"'
//  - for the second, there is only one string, the link 'http://example.org',
//     this will be stored in the result with an empty key.  If more than one of
//     these is given in the response, the last one will take precedence.
//
// The link_parser regular expression consists of two main chunks, with gratuitous
// whitespace padding:
// <([^>]*)> - this is the first capture group, for the link portion.  Since the URL
//     cannot contain '>' characters, we capture everything from the first '<' to the
//     first '>'.
// (?:;([^,]+)(?:,|$) - this is the optional remainder of the link (called the param
//     in the RFC).  We match the semicolon, followed by the param, followed by either
//     a comma or the end of the line.  Like before, the capture group is configured
//     to only capture the param. Note: this will fail if the param contains a comma.
header_parser_singleton_t::header_parser_singleton_t() :
    link_parser("^\\s*<([^>]*)>\\s*(?:;\\s*([^,]+)\\s*(?:,|$))", RE2::Quiet)
{
    memset(&settings, 0, sizeof(settings));
    settings.on_header_field = &header_parser_singleton_t::on_header_field;
    settings.on_header_value = &header_parser_singleton_t::on_header_value;
    settings.on_headers_complete = &header_parser_singleton_t::on_headers_complete;
}

void header_parser_singleton_t::initialize() {
    if (instance == nullptr) {
        instance = new header_parser_singleton_t();
    }

    instance->current_field.clear();
    instance->link_headers.clear();
    http_parser_init(&instance->parser, HTTP_RESPONSE);
    instance->parser.data = instance;
}

ql::datum_t
header_parser_singleton_t::parse(const std::string &header) {
    initialize();
    size_t res = http_parser_execute(&instance->parser,
                                     &instance->settings,
                                     header.data(),
                                     header.length());

    if (instance->parser.upgrade) {
        // Upgrade header was present - error
        throw curl_exc_t("upgrade header present in response headers");
    } else if (res != header.length()) {
        throw curl_exc_t(strprintf("unknown error when parsing response headers:\n%s\n",
                                   header.c_str()));
    }

    // Return an empty object if we didn't receive any headers
    if (instance->header_fields.size() == 0 &&
        instance->link_headers.size() == 0) {
        return ql::datum_t::empty_object();
    } else if (instance->link_headers.size() > 0) {
        // Include the specially-parsed link header field
        instance->header_fields[datum_string_t("link")] =
            ql::datum_t(std::move(instance->link_headers));
    }

    ql::datum_t parsed_header = ql::datum_t(std::move(instance->header_fields));
    instance->header_fields.clear();
    return parsed_header;
}

int header_parser_singleton_t::on_headers_complete(UNUSED http_parser *parser) {
    // Always stop once we've parsed the headers
    return 1;
}

int header_parser_singleton_t::on_header_field(http_parser *parser,
                                               const char *field,
                                               size_t length) {
    guarantee(instance == parser->data);
    instance->current_field.assign(field, length);

    // Lowercase header fields so they are more reliable
    for (size_t i = 0; i < instance->current_field.length(); ++i) {
        instance->current_field[i] = tolower(instance->current_field[i]);
    }
    return 0;
}

int header_parser_singleton_t::on_header_value(http_parser *parser,
                                               const char *value,
                                               size_t length) {
    guarantee(instance == parser->data);
    // Special handling for 'Link' headers to make them usable in the query language
    if (instance->current_field == "link") {
        instance->add_link_header(std::string(value, length));
    } else {
        instance->header_fields[datum_string_t(instance->current_field)] =
            ql::datum_t(datum_string_t(length, value));
    }
    return 0;
}

void header_parser_singleton_t::add_link_header(const std::string &line) {
    // Capture the link and its parameter
    re2::StringPiece line_re2(line);
    std::string value;
    std::string param;
    while (RE2::FindAndConsume(&line_re2, link_parser, &value, &param)) {
        link_headers[datum_string_t(param)] =
            ql::datum_t(datum_string_t(value));
    }

    if (line_re2.length() != 0) {
        // The entire string was not consumed, meaning something does not match
        throw curl_exc_t(strprintf("failed to parse link header line: '%s'",
                                   line.c_str()));
    }
}

void parse_header(const std::string &header,
                  http_result_t *res_out) {
    res_out->header = header_parser_singleton_t::parse(header);
}

void save_cookies(CURL *curl_handle, http_result_t *res_out) {
    struct curl_slist *cookies;
    CURLcode curl_res = curl_easy_getinfo(curl_handle, CURLINFO_COOKIELIST, &cookies);

    if (curl_res != CURLE_OK) {
        throw curl_exc_t("failed to get a list of cookies from the session");
    }

    res_out->cookies.clear();
    for (curl_slist *current = cookies; current != nullptr; current = current->next) {
        res_out->cookies.push_back(std::string(current->data));
    }

    if (cookies != nullptr) {
        curl_slist_free_all(cookies);
    }
}

void json_to_datum(const std::string &json,
                   const ql::configured_limits_t &limits,
                   reql_version_t reql_version,
                   attach_json_to_error_t attach_json,
                   http_result_t *res_out) {
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (!doc.HasParseError()) {
        res_out->body = ql::to_datum(doc, limits, reql_version);
    } else {
        res_out->error.assign(
            strprintf("failed to parse JSON response: %s",
                      rapidjson::GetParseError_En(doc.GetParseError())));
        if (attach_json == attach_json_to_error_t::YES) {
            res_out->body = ql::datum_t(datum_string_t(json));
        }
    }
}

// This class is created on-demand, but at most once per extproc.  There is
// no code path to destroy this object, but memory usage shouldn't be terrible,
// and it will be cleaned up when the extproc is shut down.
class jsonp_parser_singleton_t {
public:
    static bool parse(const std::string &jsonp, std::string *json_string) {
        initialize_if_needed();
        return RE2::FullMatch(jsonp, instance->parser1, json_string) ||
               RE2::FullMatch(jsonp, instance->parser2, json_string) ||
               RE2::FullMatch(jsonp, instance->parser3, json_string);
    }
private:
    // The JSONP specification at www.json-p.org provides the following formats:
    // 1. functionName(JSON);
    // 2. obj.functionName(JSON);
    // 3. obj["function-name"](JSON);
    // The following regular expressions will parse out the JSON section from each format
    jsonp_parser_singleton_t() :
        parser1(std::string(js_ident) + fn_call + expr_end, RE2::Quiet),
        parser2(std::string(js_ident) + "\\." + js_ident + fn_call + expr_end, RE2::Quiet),
        parser3(std::string(js_ident) + "\\[\\s*['\"].*['\"]\\s*\\]\\s*" +
                fn_call + expr_end, RE2::Quiet)
    { }

    static void initialize_if_needed() {
        if (instance == nullptr) {
            instance = new jsonp_parser_singleton_t();
        }
    }

    static jsonp_parser_singleton_t *instance;
    // Matches a javascript identifier with whitespace on either side
    static const char *js_ident;
    // Matches the function call with a capture group for the JSON data
    static const char *fn_call;
    // Matches the end of the javascript expression, whitespace and an optional semicolon
    static const char *expr_end;

    RE2 parser1;
    RE2 parser2;
    RE2 parser3;
};

// Static const initializations for above
jsonp_parser_singleton_t *jsonp_parser_singleton_t::instance = nullptr;
const char *jsonp_parser_singleton_t::fn_call = "\\(((?s).*)\\)";
const char *jsonp_parser_singleton_t::expr_end = "\\s*;?\\s*";
const char *jsonp_parser_singleton_t::js_ident =
    "\\s*"
    "[$_\\p{Ll}\\p{Lt}\\p{Lu}\\p{Lm}\\p{Lo}\\p{Nl}]"
    "[$_\\p{Ll}\\p{Lt}\\p{Lu}\\p{Lm}\\p{Lo}\\p{Nl}\\p{Mn}\\p{Mc}\\p{Nd}\\p{Pc}]*"
    "\\s*";

void jsonp_to_datum(const std::string &jsonp, const ql::configured_limits_t &limits,
                    reql_version_t reql_version,
                    attach_json_to_error_t attach_json,
                    http_result_t *res_out) {
    std::string json_string;
    if (jsonp_parser_singleton_t::parse(jsonp, &json_string)) {
        json_to_datum(json_string, limits, reql_version, attach_json, res_out);
    } else {
        res_out->error.assign("failed to parse JSONP response");
        if (attach_json == attach_json_to_error_t::YES) {
            res_out->body = ql::datum_t(datum_string_t(jsonp));
        }
    }
}
