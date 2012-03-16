#ifndef HTPP_HTPP_HPP_
#define HTPP_HTPP_HPP_

#include "errors.hpp"
#include <boost/fusion/include/io.hpp>
#include <boost/scoped_ptr.hpp>
#include <sstream>

#include "arch/types.hpp"
#include "parsing/util.hpp"

enum http_method_t {
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

struct query_parameter_t {
    std::string key;
    std::string val;
};

struct header_line_t {
    std::string key;
    std::string val;
};

struct http_req_t {
    http_method_t method;
    std::string resource;
    std::vector<query_parameter_t> query_params;
    std::string version;
    std::vector<header_line_t> header_lines;
    std::string body;

    std::string find_query_param(const std::string&) const;
    std::string find_header_line(const std::string&) const;
    bool has_header_line(const std::string&) const;
};

int content_length(http_req_t);

struct http_res_t {
    std::string version;
    int code;
    std::vector<header_line_t> header_lines;
    std::string body;

    void add_header_line(const std::string&, const std::string&);
    void set_body(const std::string&, const std::string&);

    http_res_t();
    explicit http_res_t(int rescode);
    void add_last_modified(int);
};

void test_header_parser();

class tcp_http_msg_parser_t {
public:
    tcp_http_msg_parser_t() {}
    bool parse(tcp_conn_t *conn, http_req_t *req);
private:
    struct version_parser_t {
        std::string version;

        bool parse(std::string &src);
    };

    struct resource_string_parser_t {
        std::string resource;
        std::vector<query_parameter_t> query_params;

        bool parse(std::string &src);
    };

    struct header_line_parser_t {
        std::string key;
        std::string val;

        bool parse(std::string &src);
    };
};

class http_app_t {
public:
    virtual http_res_t handle(const http_req_t &) = 0;
protected:
    virtual ~http_app_t() { }
};

/* creating an http server will bind to the specified port and listen for http
 * connections, the data from incoming connections will be parsed into
 * http_req_ts and passed to the handle function which must then return an http
 * msg that's a meaningful response */
class http_server_t {
public:
    http_server_t(int port, http_app_t *application);
private:
    void handle_conn(boost::scoped_ptr<nascent_tcp_conn_t> &conn);
    boost::scoped_ptr<tcp_listener_t> tcp_listener;
    http_app_t *application;
};

std::string percent_escaped_string(const std::string &s);

std::string percent_unescaped_string(const std::string &s) THROWS_ONLY(std::runtime_error);

#endif
