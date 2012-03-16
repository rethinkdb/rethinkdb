#include "spirit/boost_parser.hpp"
#include "arch/arch.hpp"
#include "logger.hpp"

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
    logINF("Got an int: %d\n", n);
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
    logINF("Val = %d\n", val);
}

void echo_conn(boost::scoped_ptr<tcp_conn_t> &conn) {
    conn->write("foo", strlen("foo"));
    conn->shutdown_write();

    for (tcp_conn_t::iterator it = conn->begin(); it != conn->end(); ++it)
        printf("%c", *it);
}
