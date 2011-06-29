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
    std::vector<std::string> header_lines;
    std::string body;
};

BOOST_FUSION_ADAPT_STRUCT(
     http_msg_t,
     (http_method_t, method)
     (std::string, resource)
     (std::string, version)
     (std::vector<std::string>, header_lines)
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
        version %= (+(char_ - space));
        //key_val_pair %= ((+(char_ - space)) >> just_space >> (+(char_ - space)));
        key_val_pair %= (+(char_ - CRLF));
        body %= (+char_);
        CRLF %= lit("\r\n") || lit("\n");

        start %= method >> just_space >> resource >> just_space >> version >> CRLF >>
                 (key_val_pair % CRLF) >>
                 CRLF >>
                 body;
        //start %= method >> just_space >> resource >> version >> *(key_val_pair) >> body;
    }
    qi::rule<Iterator> just_space;
    qi::rule<Iterator, http_method_t()> method;
    qi::rule<Iterator, std::string()> resource;
    qi::rule<Iterator, std::string()> version;
    qi::rule<Iterator, std::string()> key_val_pair;
    qi::rule<Iterator, std::string()> body;
    qi::rule<Iterator> CRLF;
    qi::rule<Iterator, http_msg_t()> start;
};

void test_header_parser();
#endif
