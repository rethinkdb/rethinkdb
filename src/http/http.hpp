// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef HTTP_HTTP_HPP_
#define HTTP_HTTP_HPP_

#include <map>
#include <string>
#include <stdexcept>
#include <vector>
#include <set>

#include "errors.hpp"
#include <boost/tokenizer.hpp>
#include <boost/shared_array.hpp>
#include <boost/optional.hpp>

#include "arch/address.hpp"
#include "arch/io/openssl.hpp"
#include "arch/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "containers/scoped.hpp"
#include "parsing/util.hpp"

enum class http_method_t {
    HEAD = 0,
    GET,
    POST,
    PUT,
    DELETE,
    TRACE,
    OPTIONS,
    CONNECT,
    PATCH
};

struct http_req_t {
    class resource_t {
    public:
        typedef boost::tokenizer<boost::char_separator<char>, char *> tokenizer;
        typedef tokenizer::iterator iterator;

        resource_t();
        explicit resource_t(const std::string &_val);
        resource_t(const char* _val, size_t size);

        MUST_USE bool assign(const std::string &_val);
        MUST_USE bool assign(const char* _val, size_t size);
        iterator begin() const;
        iterator end() const;
        std::string as_string(const iterator &from) const;
        std::string as_string() const;
    private:
        resource_t(const resource_t &from, const iterator &resource_start);
        friend struct http_req_t;

        // This function returns a pointer to the beginning of the token (without the leading '/')
        const char* token_start_position(const iterator& it) const;

        boost::shared_array<char> val;
        size_t val_size;
        iterator b;
        iterator e;
    } resource;

    ip_and_port_t peer;

    http_method_t method;
    std::map<std::string, std::string> query_params;
    std::string version;
    std::map<std::string, std::string> header_lines;
    std::string body;
    std::string get_sanitized_body() const;

    http_req_t();
    explicit http_req_t(const std::string &resource_path);
    http_req_t(const http_req_t &from, const resource_t::iterator& resource_start);

    boost::optional<std::string> find_query_param(const std::string&) const;
    boost::optional<std::string> find_header_line(const std::string&) const;
    void add_header_line(const std::string&, const std::string&);
    bool has_header_line(const std::string&) const;
};

int content_length(const http_req_t&);

enum class http_status_code_t {
    OK = 200,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500
};

class http_res_t {
public:
    std::string version;
    http_status_code_t code;
    std::map<std::string, std::string> header_lines;
    std::string body;

    void add_header_line(const std::string&, const std::string&);
    void set_body(const std::string&, const std::string&);

    http_res_t();
    explicit http_res_t(http_status_code_t rescode);
    http_res_t(http_status_code_t rescode, const std::string &content_type,
               const std::string &content);
};

bool maybe_gzip_response(const http_req_t &req, http_res_t *res);

http_res_t http_error_res(const std::string &content,
    http_status_code_t rescode = http_status_code_t::BAD_REQUEST);

class tcp_http_msg_parser_t {
public:
    tcp_http_msg_parser_t() {}
    bool parse(tcp_conn_t *conn, http_req_t *req, signal_t *closer) THROWS_ONLY(tcp_conn_read_closed_exc_t);
private:
    struct version_parser_t {
        std::string version;

        bool parse(const std::string &src);
    };

    struct resource_string_parser_t {
        std::string resource;
        std::map<std::string, std::string> query_params;

        bool parse(const std::string &src);
    };

    struct header_line_parser_t {
        std::string key;
        std::string val;

        bool parse(const std::string &src);
    };
};

class http_app_t {
public:
    virtual void handle(const http_req_t &request,
                        http_res_t *result,
                        signal_t *interruptor) = 0;
protected:
    virtual ~http_app_t() { }
};

/* creating an http server will bind to the specified port and listen for http
 * connections, the data from incoming connections will be parsed into
 * http_req_ts and passed to the handle function which must then return an http
 * msg that's a meaningful response */
class http_server_t {
public:
    http_server_t(
        tls_ctx_t *tls_ctx, const std::set<ip_address_t> &local_addresses,
        int port, http_app_t *application);
    ~http_server_t();
    int get_port() const;
private:
    void handle_conn(const scoped_ptr_t<tcp_conn_descriptor_t> &conn, auto_drainer_t::lock_t);
    http_app_t *application;
    auto_drainer_t auto_drainer;
    scoped_ptr_t<tcp_listener_t> tcp_listener;
    tls_ctx_t *tls_ctx;
};

std::string percent_escaped_string(const std::string &s);

bool percent_unescape_string(const std::string &s, std::string *out);

std::string http_format_date(const time_t date);

#endif /* HTTP_HTTP_HPP_ */
