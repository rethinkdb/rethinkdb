// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "extproc/http_job.hpp"

#include <ctype.h>
#include <curl/curl.h>

#include <re2/re2.h>

#include <limits>

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "extproc/extproc_job.hpp"

#define RETHINKDB_USER_AGENT (SOFTWARE_NAME_STRING "/" RETHINKDB_VERSION)

void parse_header(std::string &&header,
                  http_result_t *res_out);

void json_to_datum(const std::string &json,
                   http_result_t *res_out);
void json_to_datum(std::string &&json,
                   http_result_t *res_out);

void jsonp_to_datum(const std::string &jsonp,
                    http_result_t *res_out);
void jsonp_to_datum(std::string &&jsonp,
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
        handle(curl_easy_init()) {
    }

    ~scoped_curl_handle_t() {
        curl_easy_cleanup(handle);
    }

    CURL *get() {
        return handle;
    }

private:
    CURL *handle;
};

// Used for adding headers, which cannot be freed until after the request is done
class scoped_curl_slist_t {
public:
    scoped_curl_slist_t() :
        slist(NULL) { }

    ~scoped_curl_slist_t() {
        if (slist != NULL) {
            curl_slist_free_all(slist);
        }
    }

    curl_slist *get() {
        return slist;
    }

    void add(const std::string &str) {
        slist = curl_slist_append(slist, str.c_str());
        if (slist == NULL) {
            throw curl_exc_t("appending headers, allocation failure");
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

    scoped_curl_slist_t headers;

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
};

size_t curl_data_t::write_body(char *ptr, size_t size, size_t nmemb, void* instance) {
    curl_data_t *self = static_cast<curl_data_t *>(instance);
    return self->write_body_internal(ptr,
                                     multiply_with_overflow_check(size, nmemb,
                                                                  "write body"));
};

size_t curl_data_t::read(char *ptr, size_t size, size_t nmemb, void* instance) {
    curl_data_t *self = static_cast<curl_data_t *>(instance);
    return self->read_internal(ptr, multiply_with_overflow_check(size, nmemb, "read"));
};

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

void http_job_t::http(const http_opts_t *opts,
                      http_result_t *res_out) {
    write_message_t msg;
    serialize(&msg, *opts);
    {
        int res = send_write_message(extproc_job.write_stream(), &msg);
        if (res != 0) { throw http_worker_exc_t("failed to send data to the worker"); }
    }

    archive_result_t res = deserialize(extproc_job.read_stream(), res_out);
    if (bad(res)) {
        throw http_worker_exc_t(strprintf("failed to deserialize result from worker "
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
        archive_result_t res = deserialize(stream_in, &opts);
        if (bad(res)) { return false; }
    }

    http_result_t result;

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
    serialize(&msg, result);
    int res = send_write_message(stream_out, &msg);
    if (res != 0) { return false; }

    return true;
}

std::string exc_encode(CURL *curl_handle, const std::string &str) {
    std::string res;

    if (str.size() != 0) {
        char *curl_res = curl_easy_escape(curl_handle, str.data(), str.size());
        if (curl_res == NULL) {
            throw curl_exc_t("encode string");
        }
        res.assign(curl_res);
        curl_free(curl_res);
    }

    return res;
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
        const std::vector<std::pair<std::string, std::string> > &fields) {
    std::string data;
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        if (it != fields.begin()) {
            data.push_back('&');
        }
        url_encode_kv(curl_handle, it->first, it->second, &data);
    }
    return data;
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
        const std::vector<std::pair<std::string, std::string> > &url_params,
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
        curl_data->headers.add(*it);
    }

    if (curl_data->headers.get() != NULL) {
        exc_setopt(curl_handle, CURLOPT_HTTPHEADER, curl_data->headers.get(), "HEADER");
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

    exc_setopt(curl_handle, CURLOPT_ENCODING, "deflate=1;gzip=0.5", "PROTOCOLS");

    exc_setopt(curl_handle, CURLOPT_NOSIGNAL, 1, "NOSIGNAL");

    // Use the proxy set when launched
    if (!proxy.empty()) {
        exc_setopt(curl_handle, CURLOPT_PROXY, proxy.c_str(), "PROXY");
    }
}

// TODO: implement depaginate
// TODO: implement streams
void perform_http(http_opts_t *opts, http_result_t *res_out) {
    scoped_curl_handle_t curl_handle;
    curl_data_t curl_data;

    if (curl_handle.get() == NULL) {
        res_out->error.assign("initialization");
        return;
    }

    try {
        set_default_opts(curl_handle.get(), opts->proxy, curl_data);
        transfer_opts(opts, curl_handle.get(), &curl_data);
    } catch (const curl_exc_t &ex) {
        res_out->error.assign(ex.what());
        return;
    }

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
            parse_header(std::move(header_data), res_out);
        }
        if (!body_data.empty()) {
            res_out->body = make_counted<const ql::datum_t>(std::move(body_data));
        }
        res_out->error = strprintf("status code %ld", response_code);
    } else {
        parse_header(std::move(header_data), res_out);

        // If this was a HEAD request, we should not be handling data, just return R_NULL
        // so the user knows the request succeeded
        if (opts->method == http_method_t::HEAD) {
            res_out->body = make_counted<const ql::datum_t>(ql::datum_t::R_NULL);
            return;
        }

        rassert(res_out->error.empty());

        switch (opts->result_format) {
        case http_result_format_t::AUTO:
            {
                std::string content_type;
                char *content_type_buffer = NULL;
                curl_easy_getinfo(curl_handle.get(),
                                  CURLINFO_CONTENT_TYPE,
                                  &content_type_buffer);

                if (content_type_buffer != NULL) {
                    content_type.assign(content_type_buffer);
                }

                for (size_t i = 0; i < content_type.length(); ++i) {
                    content_type[i] = tolower(content_type[i]);
                }

                if (content_type.find("application/json") == 0) {
                    json_to_datum(std::move(body_data), res_out);
                } else if (content_type.find("text/javascript") == 0 ||
                           content_type.find("application/json-p") == 0 ||
                           content_type.find("text/json-p") == 0) {
                    // Try to parse the result as JSON, then as JSONP, then plaintext
                    // Do not use move semantics here, as we retry on errors
                    json_to_datum(body_data, res_out);
                    if (!res_out->error.empty()) {
                        res_out->error.clear();
                        jsonp_to_datum(body_data, res_out);
                        if (!res_out->error.empty()) {
                            res_out->error.clear();
                            res_out->body =
                                make_counted<const ql::datum_t>(std::move(body_data));
                        }
                    }
                } else {
                    res_out->body =
                        make_counted<const ql::datum_t>(std::move(body_data));
                }
            }
            break;
        case http_result_format_t::JSON:
            json_to_datum(std::move(body_data), res_out);
            break;
        case http_result_format_t::JSONP:
            jsonp_to_datum(std::move(body_data), res_out);
            break;
        case http_result_format_t::TEXT:
            res_out->body = make_counted<const ql::datum_t>(std::move(body_data));
            break;
        default:
            unreachable();
        }
    }
}

void parse_header(std::string &&header,
                  http_result_t *res_out) {
    // TODO: implement this
    res_out->header = make_counted<const ql::datum_t>(std::move(header));
}

// This version will not use move semantics for the body data, to be used in
// result_format="auto", where we may fallback to jsonp
void json_to_datum(const std::string &json,
                   http_result_t *res_out) {
    scoped_cJSON_t cjson(cJSON_Parse(json.c_str()));
    if (cjson.get() != NULL) {
        res_out->body = make_counted<const ql::datum_t>(cjson);
    } else {
        res_out->error.assign("failed to parse JSON response");
    }
}

// This version uses move semantics and includes the body as a string when a
// parsing error occurs.
void json_to_datum(std::string &&json,
                   http_result_t *res_out) {
    scoped_cJSON_t cjson(cJSON_Parse(json.c_str()));
    if (cjson.get() != NULL) {
        res_out->body = make_counted<const ql::datum_t>(cjson);
    } else {
        res_out->error.assign("failed to parse JSON response");
        res_out->body = make_counted<const ql::datum_t>(std::move(json));
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
        parser1(std::string(js_ident) + fn_call + expr_end),
        parser2(std::string(js_ident) + "\\." + js_ident + fn_call + expr_end),
        parser3(std::string(js_ident) + "\\[\\s*['\"].*['\"]\\s*\\]\\s*" +
                fn_call + expr_end)
    { }

    static void initialize_if_needed() {
        if (instance == NULL) {
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
jsonp_parser_singleton_t *jsonp_parser_singleton_t::instance = NULL;
const char *jsonp_parser_singleton_t::fn_call = "\\(((?s).*)\\)";
const char *jsonp_parser_singleton_t::expr_end = "\\s*;?\\s*";
const char *jsonp_parser_singleton_t::js_ident =
    "\\s*"
    "[$_\\p{Ll}\\p{Lt}\\p{Lu}\\p{Lm}\\p{Lo}\\p{Nl}]"
    "[$_\\p{Ll}\\p{Lt}\\p{Lu}\\p{Lm}\\p{Lo}\\p{Nl}\\p{Mn}\\p{Mc}\\p{Nd}\\p{Pc}]*"
    "\\s*";

// This version will not use move semantics for the body data, to be used in
// result_format="auto", where we may fallback to passing back the body as a string
void jsonp_to_datum(const std::string &jsonp, http_result_t *res_out) {
    std::string json_string;
    if (jsonp_parser_singleton_t::parse(jsonp, &json_string)) {
        json_to_datum(json_string, res_out);
    } else {
        res_out->error.assign("failed to parse JSONP response");
    }
}


// This version uses move semantics and includes the body as a string when a
// parsing error occurs.
void jsonp_to_datum(std::string &&jsonp, http_result_t *res_out) {
    std::string json_string;
    if (jsonp_parser_singleton_t::parse(jsonp, &json_string)) {
        json_to_datum(std::move(json_string), res_out);
    } else {
        res_out->error.assign("failed to parse JSONP response");
        res_out->body = make_counted<const ql::datum_t>(std::move(jsonp));
    }
}
