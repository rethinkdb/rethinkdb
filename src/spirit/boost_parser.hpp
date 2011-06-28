#ifndef BOOST_PARSER_HPP
#define BOOST_PARSER_HPP

#include "arch/arch.hpp"

template<typename Expr>
void parse_connection(boost::scoped_ptr<tcp_conn_t>, Expr const&);

void parse_ints(boost::scoped_ptr<tcp_conn_t> &);

/* Parse ints is an example of parser that can parse data coming off of a tcp
 * connection to run it put the line below somewhere in the startup code of */

//tcp_listener_t int_parser(2222, boost::bind(parse_ints, _1));

#endif
