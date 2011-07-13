#include "spirit/boost_parser.hpp"
#include "arch/arch.hpp"
#include <vector>

#include <boost/config/warning_disable.hpp>
//[tutorial_adder_includes
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <iostream>
#include <string>
//]

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

using qi::int_;
using qi::_1;
using ascii::space;
using phoenix::ref;

template<typename Expr>
void parse_connection(boost::scoped_ptr<tcp_conn_t> conn, Expr const& expr) {
    return boost::spirit::qi::parse(conn->begin(), conn->end(), expr);
}

void print_int(int n) {
    (void)n;
    //logINF("Got an int: %d\n", n);
}

void parse_ints(boost::scoped_ptr<tcp_conn_t> &conn) {
    int val = 0;
    tcp_conn_t::iterator begin = conn->begin(), end = conn->end();
    qi::phrase_parse(begin, end,
            (
             '(' >> (int_[ref(val) += qi::_1])
                 >> *(',' >> int_[ref(val) += qi::_1])
                 >> ')')
            ,
            space);
    (void)val;
    //logINF("Val = %d\n", val);
}

