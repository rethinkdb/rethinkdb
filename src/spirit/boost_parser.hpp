#ifndef BOOST_PARSER_HPP
#define BOOST_PARSER_HPP

#define BOOST_SPIRIT_DEBUG_PRINT_SOME 1

#include "arch/arch.hpp"
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/classic_debug.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>


namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;
namespace fusion = boost::fusion;
namespace ascii = boost::spirit::ascii;

template<typename Expr>
void parse_connection(boost::scoped_ptr<tcp_conn_t>, Expr const&);

void parse_ints(boost::scoped_ptr<tcp_conn_t> &);

void echo_conn(boost::scoped_ptr<tcp_conn_t> &);

/* Parse ints is an example of parser that can parse data coming off of a tcp
 * connection to run it put the line below somewhere in the startup code of */

//tcp_listener_t int_parser(2222, boost::bind(parse_ints, _1));

#endif
