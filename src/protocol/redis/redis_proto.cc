#include "protocol/redis/redis_proto.hpp"
#include "protocol/redis/redis.hpp"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_match.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/home/phoenix/bind/bind_function.hpp> 
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include <vector>
#include <iostream>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace px = boost::phoenix;

//#define BOOST_SPIRIT_DEBUG
#define BOOST_SPIRIT_DEBUG_FLAGS_NODES
#define SPIRIT_DEBUG(RULE) RULE.name(#RULE); debug(RULE);

#define COMMAND_1(CNAME, OUTPUT) CNAME = command(std::string(#CNAME), 1)\
                [px::bind(&redis_grammar::OUTPUT, this, &redis_interface_t::CNAME, qi::_1[0])]
#define COMMAND_2(CNAME, OUTPUT) CNAME = command(std::string(#CNAME), 2)\
                [px::bind(&redis_grammar::OUTPUT, this, &redis_interface_t::CNAME, qi::_1[0], qi::_1[1])]
#define COMMAND_N(CNAME, OUTPUT) CNAME = command_n(std::string(#CNAME))\
                [px::bind(&redis_grammar::OUTPUT, this, &redis_interface_t::CNAME, qi::_1)]

template <typename Iterator>
struct redis_grammar : qi::grammar<Iterator>, redis_interface_t {
    redis_grammar(tcp_conn_t *conn, std::iostream *redis_stream, get_store_t *get_store, set_store_interface_t *set_store) :
        redis_grammar::base_type(start),
        redis_interface_t(get_store, set_store),
        out_stream(redis_stream),
        out_conn(conn)
    {
        eol = qi::lit("\r\n");
        arg_num %= qi::uint_;
        arg_bytes %= qi::uint_;
        data %= qi::repeat(qi::_r1)[qi::char_];
        arg = '$' >> arg_bytes[qi::_a = qi::_1] >> eol >> data(qi::_a)[qi::_val = qi::_1] >> eol;
        args = '*' >> arg_num[qi::_a = qi::_1] >> eol >> qi::repeat(qi::_a)[arg];

        command = '*' >> qi::uint_(qi::_r2 + 1) >> eol
                >> '$' >> qi::uint_(px::size(qi::_r1)) >> eol
                >> qi::no_case[qi::string(qi::_r1)] >> eol
                >> (qi::repeat(qi::_r2)[arg])[qi::_val = qi::_1];
        command_n = '*' >> qi::uint_[qi::_a = qi::_1 - 1] >> eol
                >> '$' >> qi::uint_(px::size(qi::_r1)) >> eol
                >> qi::no_case[qi::string(qi::_r1)] >> eol
                >> (qi::repeat(qi::_a)[arg])[qi::_val = qi::_1];

        //Commands
        COMMAND_2(set, output_status_result);
        COMMAND_1(get, output_bulk_result);
        COMMAND_1(incr, output_integer_result);
        COMMAND_1(decr, output_integer_result);
        COMMAND_N(mget, output_multi_bulk_result);

        commands = set | get | incr | decr | mget;
        start = commands;
    }

private:
    std::ostream *out_stream;
    tcp_conn_t *out_conn;

    //Support rules
    qi::rule<Iterator> eol;
    qi::rule<Iterator, unsigned()> arg_num;
    qi::rule<Iterator, unsigned()> arg_bytes;
    qi::rule<Iterator, std::vector<char>(unsigned)> data;
    qi::rule<Iterator, std::vector<char>(), qi::locals<unsigned> > arg;
    qi::rule<Iterator, unsigned(), std::vector<std::vector<char> >, qi::locals<unsigned> > args;
    qi::rule<Iterator, std::vector<std::vector<char> >(std::string, unsigned)> command;
    qi::rule<Iterator, std::vector<std::vector<char> >(std::string), qi::locals<unsigned> > command_n;

    //Commands
    qi::rule<Iterator> set;
    qi::rule<Iterator> get;
    qi::rule<Iterator> incr;
    qi::rule<Iterator> decr;
    qi::rule<Iterator> mget;
    
    qi::rule<Iterator> commands;
    qi::rule<Iterator> start;

    //Output
    void output_status_result(const boost::shared_ptr<status_result>
            (redis_interface_t::*method)(const std::vector<char>&, const std::vector<char>&),
            std::vector<char> &one, std::vector<char> &two) {
        const boost::shared_ptr<status_result> res = (this->*method)(one, two);
        if(res->status == OK) out_conn->write("+", 1);
        else                  out_conn->write("-", 1);
        out_conn->write(res->msg, strlen(res->msg));
        out_conn->write("\r\n", 2);
    }
    
    void output_integer_result(integer_result (redis_interface_t::*method)(const std::vector<char>&), std::vector<char> &one) {
        integer_result res = (this->*method)(one);
        char buff[20]; //Max size of a base 10 representation of a 64 bit number
        sprintf(buff, "%d", res);
        out_conn->write(":", 1);
        out_conn->write(buff, strlen(buff));
        out_conn->write("\r\n", 2);
    }

    void output_result_vector(const std::vector<char> &one) {
        char buff[20];
        sprintf(buff, "%d", (int)one.size());

        out_conn->write("$", 1);
        out_conn->write(buff, strlen(buff));
        out_conn->write("\r\n", 2);
        out_conn->write((char *)(&one.at(0)), one.size());
        out_conn->write("\r\n", 2);
    }

    void output_bulk_result(const boost::shared_ptr<bulk_result>
            (redis_interface_t::*method)(const std::vector<char>&),
            std::vector<char> &one) {
        const boost::shared_ptr<bulk_result> res = (this->*method)(one);
        output_result_vector(*res);     
    }

    void output_multi_bulk_result(const boost::shared_ptr<multi_bulk_result>
            (redis_interface_t::*method)(const std::vector<std::vector<char> >&),
            std::vector<std::vector<char> > &one) {
        const boost::shared_ptr<multi_bulk_result> res = (this->*method)(one);
        char buff[20];
        sprintf(buff, "%d", (int)res->size());

        out_conn->write("*", 1);
        out_conn->write(buff, strlen(buff));
        out_conn->write("\r\n", 2);
        for(std::vector<std::vector<char> >::iterator iter = res->begin(); iter != res->end(); ++iter) {
            output_result_vector(*iter);
        }
    }

};

void serve_redis(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store) {
    redis_grammar<tcp_conn_t::iterator> redis(conn, NULL, get_store, set_store);
    try {
        while(true) {
            qi::parse(conn->begin(), conn->end(), redis);
            //if(!r) {
                const_charslice res = conn->peek();
                size_t len = res.end - res.beg;
                conn->pop(len);
            //}
        }
    } catch(tcp_conn_t::read_closed_exc_t) {}
}

void serve_redis(std::iostream &redis_stream, get_store_t *get_store, set_store_interface_t *set_store) {
    redis_grammar<boost::spirit::basic_istream_iterator<char> > redis(NULL, &redis_stream, get_store, set_store);
    //qi::detail::match_manip<redis_grammar<boost::spirit::basic_istream_iterator<char, std::char_traits<char> > >, mpl_::bool_<false>, mpl_::bool_<false>, boost::spirit::unused_type, const boost::spirit::unused_type> match = qi::match(redis);
    while(redis_stream.good()) {
                        //TODO construct the manipulator outside of the loop?
        redis_stream >> qi::match(redis);
        if(redis_stream.fail()) {
            std::cout << "Bad parse, will I have to now extract the bad data?" << std::endl;
        }
    }
}
