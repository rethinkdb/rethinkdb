#ifndef __HTPP_HTPP_HPP__
#define __HTPP_HTPP_HPP__

#include "spirit/boost_parser.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>


enum http_method_t {
    GET = 0,
    POST
};

struct header_line_t {
    std::string key;
    std::string val;
};

BOOST_FUSION_ADAPT_STRUCT(
        header_line_t,
        (std::string, key)
        (std::string, val)
)

struct http_msg_t {
    http_method_t method;
    std::string resource;
    std::string version;
    std::vector<header_line_t> header_lines;
    std::string body;
};

BOOST_FUSION_ADAPT_STRUCT(
     http_msg_t,
     (http_method_t, method)
     (std::string, resource)
     (std::string, version)
     (std::vector<header_line_t>, header_lines)
     (std::string, body)
)

void f_break();

template <typename Iterator>
struct http_msg_parser_t : qi::grammar<Iterator, http_msg_t()> {
    http_msg_parser_t() : http_msg_parser_t::base_type(start) {
        using qi::lit;
        using qi::_val;
        using ascii::char_;
        using ascii::space;
        using qi::_1;

        just_space %= ' ';
        method %= (lit("GET")[_val = GET] || lit("POST")[_val = POST]);
        resource %= (+(char_ - space));
        version %= lit("HTTP/") >> (+(char_ - space));
        key_val_pair %= ((+(char_ - ":")) >>":" >> just_space >> (+(char_ - CRLF)));
        body %= (+char_);
        CRLF %= lit("\r\n") || lit("\n");

        start %= method >> just_space >> resource >> just_space >> version >> CRLF >>
                 (key_val_pair % CRLF) >>
                 CRLF >>
                 body;

        //just_space.name("just_space"); debug(just_space);
        //method.name("method"); debug(method);
        //resource.name("resource"); debug(resource);
        //version.name("version"); debug(version);
        //key_val_pair.name("key_val_pair"); debug(key_val_pair);
        //body.name("body"); debug(body);
        //CRLF.name("CRLF"); debug(CRLF);
        //start.name("start"); debug(start);
    }
    qi::rule<Iterator> just_space;
    qi::rule<Iterator, http_method_t()> method;
    qi::rule<Iterator, std::string()> resource;
    qi::rule<Iterator, std::string()> version;
    qi::rule<Iterator, header_line_t()> key_val_pair;
    qi::rule<Iterator, std::string()> body;
    qi::rule<Iterator> CRLF;
    qi::rule<Iterator, http_msg_t()> start;
};


void test_header_parser();

/* creating an http server will bind to the specified port and listen for http
 * connections, the data from incoming connections will be parsed into
 * http_msg_ts and passed to the handle function which must then return an http
 * msg that's a meaningful response */
class http_server_t {
private:
    boost::scoped_ptr<tcp_listener_t> tcp_listener;
public:
    http_server_t(int);
    virtual ~http_server_t() {}
private:
    virtual http_msg_t handle(const http_msg_t &) = 0;
protected:
    void handle_conn(boost::scoped_ptr<tcp_conn_t> &conn);
};

class test_server_t : public http_server_t {
public:
    test_server_t(int);
private:
    http_msg_t handle(const http_msg_t &);
};

#endif
