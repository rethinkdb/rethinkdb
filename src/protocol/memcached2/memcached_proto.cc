#include "memcached_proto.hpp"
#include "memcached.hpp"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <string>
#include <vector>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace px = boost::phoenix;

template <typename Iterator>
struct memcached_grammar : qi::grammar<Iterator>, memcached_interface_t {
    memcached_grammar(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store) :
        memcached_grammar::base_type(start),
        memcached_interface_t(conn, get_store, set_store)
    {
        just_space = *qi::char_(' ');
        eol = qi::lit("\r\n");
        key %= qi::repeat(1, 250)[qi::as_string[qi::char_ - ascii::space]];
        flags %= qi::uint_;
        expiration %= qi::uint_;
        data %= qi::repeat(qi::_r1)[qi::char_];

        //Commands
        get = (ascii::no_case["GET"] >> just_space >> key >> eol)[px::bind(do_get, qi::_1)];
        set = (ascii::no_case["SET"] >> just_space
                >> key >> just_space
                >> flags >> just_space
                >> expiration >> just_space
                >> qi::uint_[qi::_a = qi::_1] >> just_space
                >> eol
                >> data(qi::_a)
                >> eol
              )[px::bind(do_set, qi::_1, qi::_2, qi::_3, qi::_5)];

        start = get | set;
    }

    qi::rule<Iterator> just_space;
    qi::rule<Iterator> eol;
    qi::rule<Iterator, std::string() > key;
    qi::rule<Iterator, unsigned()> flags;
    qi::rule<Iterator, unsigned()> expiration;
    qi::rule<Iterator, std::vector<char>(unsigned) > data;
    
    qi::rule<Iterator> get;
    qi::rule<Iterator, qi::locals<unsigned> > set;

    qi::rule<Iterator> start;
};

void serve_memcache2(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store) {
    memcached_interface_t memcached(conn, get_store, set_store);
    while(qi::parse(conn->begin(), conn->end(), memcached)) {}
}
