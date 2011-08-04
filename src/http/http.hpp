#ifndef __HTPP_HTPP_HPP__
#define __HTPP_HTPP_HPP__

#include "spirit/boost_parser.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_array.hpp>

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

BOOST_FUSION_ADAPT_STRUCT(
    query_parameter_t,
    (std::string, key)
    (std::string, val)
)

struct header_line_t {
    std::string key;
    std::string val;
};

BOOST_FUSION_ADAPT_STRUCT(
        header_line_t,
        (std::string, key)
        (std::string, val)
)

struct http_req_t {
    http_method_t method;
    std::string resource;
    std::vector<query_parameter_t> query_params;
    std::string version;
    std::vector<header_line_t> header_lines;
    std::string body;

    std::string find_query_param(std::string) const;
    std::string find_header_line(std::string) const;
    bool has_query_param(std::string) const;
    bool has_header_line(std::string) const;
};

int content_length(http_req_t);

BOOST_FUSION_ADAPT_STRUCT(
     http_req_t,
     (http_method_t, method)
     (std::string, resource)
     (std::vector<query_parameter_t>, query_params)
     (std::string, version)
     (std::vector<header_line_t>, header_lines)
     //(std::string, body)
)

struct http_res_header_t {
public:
    std::string version;
    int code;
protected:
    std::map<std::string, std::string> header_lines;
    time_t last_modified;
    

public:
    time_t get_last_modified() { return last_modified; }

    void add_header_line(std::string const &, std::string const &);
    void add_last_modified(time_t);

    http_res_header_t()
        : code(-1), last_modified(-1)
    { }
};

struct http_res_body_t {
    virtual std::string content_type() const = 0;
    virtual size_t content_length() const = 0;
    virtual std::string content() const = 0;

    virtual ~http_res_body_t() { }
};

struct http_res_simple_body_t : public http_res_body_t {
    std::string content_type() const;
    size_t content_length() const;
    std::string content() const;

    http_res_simple_body_t(const std::string &, const std::string &);

private:
    std::string _content_type;
    std::string _content;
};

//TODO this still has to make a copy of the data to return it as a std::string
//from content() we could change this but it's probably not worth it
struct http_res_simple_efficient_copy_body_t : public http_res_body_t {
    std::string content_type() const;
    size_t content_length() const;
    std::string content() const;
    http_res_simple_efficient_copy_body_t(const std::string &, const boost::shared_array<char> &, size_t size);

private:
    std::string _content_type;
    boost::shared_array<char> _content;
    size_t size;
};

struct http_res_multipart_body_t : public http_res_body_t {
    std::string content_type() const;
    size_t content_length() const;
    std::string content() const;

    http_res_multipart_body_t();
private:
    typedef boost::ptr_vector<http_res_body_t> body_ptr_vec_t;
    body_ptr_vec_t _content;
    std::string sep_string;

public:
    //takes responsibility for the pointer
    void add_content(http_res_body_t *);
};

struct http_res_t  : public http_res_header_t {
    boost::shared_ptr<http_res_body_t> body;
public:
    void write_conn(boost::scoped_ptr<tcp_conn_t> &);

    // this is a convenience/legacy function that just sets up a
    // http_res_simple_body_t with this content-type and content
    void set_body(std::string const &, std::string const &);
    void set_body(std::string const &, boost::shared_array<char> &, size_t);
};

/* struct http_res_t {
    std::string version;
    int code;
    std::vector<header_line_t> header_lines;
    std::string body;

    void add_header_line(std::string const &, std::string const &);
    void set_body(std::string const &, std::string const &);
    void add_last_modified(time_t);

    http_res_t()
        : code(-1), last_modified_time(-1)
    { }

private:
    time_t last_modified_time;
public:
    time_t get_last_modified_time() { return last_modified_time; }
}; */

template <typename Iterator>
struct http_msg_parser_t : qi::grammar<Iterator, http_req_t()> {
    http_msg_parser_t() : http_msg_parser_t::base_type(start) {
        using qi::lit;
        using qi::_val;
        using ascii::char_;
        using ascii::space;
        using qi::_1;
        using qi::repeat;
        namespace labels = qi::labels;
        using boost::phoenix::at_c;
        using boost::phoenix::bind;

        just_space %= ' ';
        method %=   (lit("HEAD"))[_val = HEAD] ||
                    (lit("GET"))[_val = GET] ||
                    (lit("POST"))[_val = POST] ||
                    (lit("PUT"))[_val = PUT] ||
                    (lit("DELETE"))[_val = DELETE] ||
                    (lit("TRACE"))[_val = TRACE] ||
                    (lit("OPTIONS"))[_val = OPTIONS] ||
                    (lit("CONNECT"))[_val = CONNECT] ||
                    (lit("PATCH"))[_val = PATCH];
        resource %= (+(char_ - space - "?"));
        query_parameter %= (+(char_ - "=" - space)) >> "=" >> (+(char_ - "&" - space));
        version %= lit("HTTP/") >> (+(char_ - space));
        header_line %= ((+(char_ - ":" - space)) >>":" >> just_space >> (+(char_ - CRLF)));
        body %= repeat(labels::_r1)[char_];
        CRLF %= lit("\r\n") || lit("\n");

        start %= method >> just_space >> resource >> 
                 -("?" >> ((query_parameter) % "&")) >>
                 just_space >> version >> CRLF >>
                 ((header_line) % CRLF); //CRLF //>>
                 //CRLF >>
                 //body(phoenix::bind(content_length, _val));

        //just_space.name("just_space"); debug(just_space);
        //method.name("method"); debug(method);
        //resource.name("resource"); debug(resource);
        //query_parameter.name("query_parameter"); debug(query_parameter);
        //version.name("version"); debug(version);
        //header_line.name("header_line"); debug(header_line);
        //body.name("body"); debug(body);
        //CRLF.name("CRLF"); debug(CRLF);
        //start.name("start"); debug(start);
    }
    qi::rule<Iterator> just_space;
    qi::rule<Iterator, http_method_t()> method;
    qi::rule<Iterator, std::string()> resource;
    qi::rule<Iterator, query_parameter_t()> query_parameter;
    qi::rule<Iterator, std::string()> version;
    qi::rule<Iterator, header_line_t()> header_line;
    qi::rule<Iterator, std::string(int)> body;
    qi::rule<Iterator> CRLF;
    qi::rule<Iterator, http_req_t()> start;
};


void test_header_parser();

/* creating an http server will bind to the specified port and listen for http
 * connections, the data from incoming connections will be parsed into
 * http_req_ts and passed to the handle function which must then return an http
 * msg that's a meaningful response */
class http_server_t {
private:
    boost::scoped_ptr<tcp_listener_t> tcp_listener;
public:
    http_server_t(int);
    virtual ~http_server_t() {}
private:
    virtual http_res_t handle(const http_req_t &) = 0;
protected:
    void handle_conn(boost::scoped_ptr<tcp_conn_t> &conn);
};

class test_server_t : public http_server_t {
public:
    test_server_t(int);
private:
    http_res_t handle(const http_req_t &);
};

#endif
