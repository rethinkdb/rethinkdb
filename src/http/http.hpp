#ifndef HTTP_HTTP_HPP_
#define HTTP_HTTP_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/tokenizer.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/optional.hpp>

#include "arch/types.hpp"
#include "concurrency/auto_drainer.hpp"
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
    class resource_t {
    public:
        typedef boost::tokenizer<boost::char_separator<char>, char *> tokenizer;
        typedef tokenizer::iterator iterator;

        resource_t();
        explicit resource_t(const std::string &val_);
        resource_t(const char* val_, size_t size);

        void assign(const std::string &val_);
        void assign(const char* val_, size_t size);
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

    http_method_t method;
    std::vector<query_parameter_t> query_params;
    std::string version;
    std::vector<header_line_t> header_lines;
    std::string body;

    http_req_t();
    explicit http_req_t(const std::string &resource_path);
    http_req_t(const http_req_t &from, const resource_t::iterator& resource_start);

    boost::optional<std::string> find_query_param(const std::string&) const;
    boost::optional<std::string> find_header_line(const std::string&) const;
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
    http_res_t(int rescode, const std::string & body_type, const std::string& body_text);
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

        bool parse(const std::string &src);
    };

    struct resource_string_parser_t {
        std::string resource;
        std::vector<query_parameter_t> query_params;

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
    virtual http_res_t handle(const http_req_t &) = 0;
protected:
    virtual ~http_app_t() { }
};

class scoped_cJSON_t;

class http_json_app_t : public http_app_t {
public:
    virtual void get_root(scoped_cJSON_t *json_out) = 0;
};

/* creating an http server will bind to the specified port and listen for http
 * connections, the data from incoming connections will be parsed into
 * http_req_ts and passed to the handle function which must then return an http
 * msg that's a meaningful response */
class http_server_t {
public:
    http_server_t(int port, http_app_t *application);
    ~http_server_t();
private:
    void handle_conn(const boost::scoped_ptr<nascent_tcp_conn_t> &conn, auto_drainer_t::lock_t);
    http_app_t *application;
    auto_drainer_t auto_drainer;
    boost::scoped_ptr<tcp_listener_t> tcp_listener;
};

std::string percent_escaped_string(const std::string &s);

std::string percent_unescaped_string(const std::string &s) THROWS_ONLY(std::runtime_error);

#endif /* HTTP_HTTP_HPP_ */
