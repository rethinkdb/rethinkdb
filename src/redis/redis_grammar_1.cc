#ifndef NO_REDIS
#include "redis/redis_grammar.hpp"

#include <boost/spirit/home/qi/action/action.hpp>
#include <boost/spirit/home/qi/char/char.hpp>
#include <boost/spirit/home/qi/directive/no_case.hpp>
#include <boost/spirit/home/qi/directive/repeat.hpp>
#include <boost/spirit/home/qi/directive/as.hpp>
// #include <boost/spirit/home/qi/nonterminal/rule.hpp>
// #include <boost/spirit/home/qi/nonterminal/grammar.hpp>
#include <boost/spirit/home/qi/numeric/uint.hpp>
#include <boost/spirit/home/qi/numeric/int.hpp>
// #include <boost/spirit/home/qi/numeric/real.hpp>
#include <boost/spirit/home/qi/operator/sequence.hpp>
#include <boost/spirit/home/qi/operator/alternative.hpp>
// #include <boost/spirit/home/qi/operator/kleene.hpp>
// #include <boost/spirit/home/qi/operator/plus.hpp>
// #include <boost/spirit/home/qi/operator/not_predicate.hpp>
// #include <boost/spirit/home/qi/auxiliary/eps.hpp>
// #include <boost/spirit/home/qi/parse.hpp>
// #include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/string/lit.hpp>

// #include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/spirit/include/phoenix.hpp>
// #include <boost/spirit/home/phoenix/bind/bind_function.hpp>
// #include <boost/spirit/home/phoenix/core/argument.hpp>

template <class Iterator>
void redis_grammar<Iterator>::help_construct_1() {
    eol = qi::lit("\r\n");
    args = '*' >> qi::uint_(qi::_r1) >> eol;
    args_n %= '*' >> qi::uint_ >> eol;
    arg_bytes %= '$' >> qi::uint_ >> eol;
    cname = '$' >> qi::uint_(px::size(qi::_r1)) >> eol
                >> qi::no_case[qi::string(qi::_r1)] >> eol;
    string_data %= qi::as_string[qi::repeat(qi::_r1)[qi::char_]];
    string_arg = arg_bytes[qi::_a = qi::_1] >> string_data(qi::_a)[qi::_val = qi::_1] >> eol;
    unsigned_data %= qi::uint_;
    unsigned_arg = arg_bytes >> unsigned_data[qi::_val = qi::_1] >> eol;
    int_data %= qi::int_;
    int_arg = arg_bytes >> int_data[qi::_val = qi::_1] >> eol;
    float_data %= qi::int_; //TODO figure out how to get the real number parser working
    float_arg = arg_bytes >> float_data[qi::_val = qi::_1] >> eol;

    command = args(qi::_r1) >> cname(qi::_r2);
    command_n = args_n[qi::_a = qi::_1] >> cname(qi::_r1) >> (qi::repeat(qi::_a - 1)[string_arg])[qi::_val = qi::_1];
    arbitrary_command = args_n[qi::_a = qi::_1] >> qi::repeat(qi::_a)[string_arg];
}


template class redis_grammar<tcp_conn_t::iterator>;
#endif //#ifndef NO_REDIS
