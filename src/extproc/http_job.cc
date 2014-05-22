// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "extproc/http_job.hpp"

#include <ctype.h>
#include <curl/curl.h>

#include <limits>

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "extproc/extproc_job.hpp"

#define RETHINKDB_USER_AGENT (SOFTWARE_NAME_STRING "/" RETHINKDB_VERSION)

http_result_t http_to_datum(const std::string &json, http_method_t method);
http_result_t perform_http(http_opts_t *opts);

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

    static size_t write(char *ptr, size_t size, size_t nmemb, void* instance);
    static size_t read(char *ptr, size_t size, size_t nmemb, void* instance);

    void set_send_data(std::string &&_send_data) {
        send_data.assign(std::move(_send_data));
    }

    std::string steal_recv_data() {
        return std::move(recv_data);
    }

    scoped_curl_slist_t headers;

private:
    // This is called for getting data in the response from the server
    size_t write_internal(char *ptr, size_t size);

    // This is called for writing data to the request when sending
    size_t read_internal(char *ptr, size_t size);

    size_t send_data_offset;
    std::string send_data;
    std::string recv_data;
};

size_t multiply_with_overflow_check(size_t a, size_t b, const std::string &info) {
    if (b == 0 || a > std::numeric_limits<size_t>::max() / b) {
        throw curl_exc_t(info + " data size too large");
    }
    return a * b;
}

size_t curl_data_t::write(char *ptr, size_t size, size_t nmemb, void* instance) {
    curl_data_t *self = static_cast<curl_data_t *>(instance);
    return self->write_internal(ptr, multiply_with_overflow_check(size, nmemb, "write"));
};

size_t curl_data_t::read(char *ptr, size_t size, size_t nmemb, void* instance) {
    curl_data_t *self = static_cast<curl_data_t *>(instance);
    return self->read_internal(ptr, multiply_with_overflow_check(size, nmemb, "read"));
};

size_t curl_data_t::write_internal(char *ptr, size_t size) {
    recv_data.append(ptr, size);
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

http_result_t http_job_t::http(const http_opts_t *opts) {
    write_message_t msg;
    serialize(&msg, *opts);
    {
        int res = send_write_message(extproc_job.write_stream(), &msg);
        if (res != 0) { throw http_worker_exc_t("failed to send data to the worker"); }
    }

    http_result_t result;
    archive_result_t res = deserialize(extproc_job.read_stream(), &result);
    if (bad(res)) {
        throw http_worker_exc_t(strprintf("failed to deserialize result from worker "
                                          "(%s)", archive_result_as_str(res)));
    }
    return result;
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
            result = perform_http(&opts);
        } catch (const std::exception &ex) {
            result = std::string(ex.what());
        } catch (...) {
            result = std::string("unknown error");
        }
    } else {
        result = std::string("global initialization");
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
               &curl_data_t::write, "WRITE FUNCTION");
    exc_setopt(curl_handle, CURLOPT_WRITEDATA, &curl_data, "WRITE DATA");

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
http_result_t perform_http(http_opts_t *opts) {
    scoped_curl_handle_t curl_handle;
    curl_data_t curl_data;

    if (curl_handle.get() == NULL) {
        return std::string("initialization");
    }

    try {
        set_default_opts(curl_handle.get(), opts->proxy, curl_data);
        transfer_opts(opts, curl_handle.get(), &curl_data);
    } catch (const curl_exc_t &ex) {
        return ex.what();
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
            return std::string(curl_easy_strerror(curl_res));
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

    if (opts->attempts == 0) {
        return std::string("could not perform, no attempts allowed");
    } else if (curl_res == CURLE_SEND_ERROR) {
        return std::string("error when sending data");
    } else if (curl_res == CURLE_RECV_ERROR) {
        return std::string("error when receiving data");
    } else if (curl_res == CURLE_COULDNT_CONNECT) {
        return std::string("could not connect to server");
    } else if (curl_res != CURLE_OK) {
        return strprintf("reading response code, '%s'", curl_easy_strerror(curl_res));
    } else if (response_code < 200 || response_code >= 300) {
        return strprintf("status code %ld", response_code);
    }

    counted_t<const ql::datum_t> res;
    switch (opts->result_format) {
    case http_result_format_t::AUTO:
        {
            char *content_type_buffer;
            curl_easy_getinfo(curl_handle.get(),
                              CURLINFO_CONTENT_TYPE,
                              &content_type_buffer);

            std::string content_type(content_type_buffer);
            for (size_t i = 0; i < content_type.length(); ++i) {
                content_type[i] = tolower(content_type[i]);
            }
            if (content_type.find("application/json") == 0) {
                return http_to_datum(curl_data.steal_recv_data(), opts->method);
            } else {
                return make_counted<const ql::datum_t>(curl_data.steal_recv_data());
            }
        }
        unreachable();
    case http_result_format_t::JSON:
        return http_to_datum(curl_data.steal_recv_data(), opts->method);
    case http_result_format_t::TEXT:
        return make_counted<const ql::datum_t>(curl_data.steal_recv_data());
    default:
        unreachable();
    }
    unreachable();
}

http_result_t http_to_datum(const std::string &json, http_method_t method) {
    // If this was a HEAD request, we should not be handling data, just return R_NULL
    // so the user knows the request succeeded (JSON parsing would fail on an empty body)
    if (method == http_method_t::HEAD) {
        return make_counted<const ql::datum_t>(ql::datum_t::R_NULL);
    }

    scoped_cJSON_t cjson(cJSON_Parse(json.c_str()));
    if (cjson.get() == NULL) {
        return std::string("failed to parse JSON response");
    }

    return make_counted<const ql::datum_t>(cjson);
}

