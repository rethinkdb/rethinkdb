#include "redis/redis_grammar.hpp"

#include <boost/spirit/home/qi/action/action.hpp>
// #include <boost/spirit/home/qi/char/char.hpp>
// #include <boost/spirit/home/qi/directive/no_case.hpp>
// #include <boost/spirit/home/qi/directive/repeat.hpp>
// #include <boost/spirit/home/qi/directive/as.hpp>
#include <boost/spirit/home/qi/nonterminal/rule.hpp>
#include <boost/spirit/home/qi/nonterminal/grammar.hpp>
// #include <boost/spirit/home/qi/numeric/uint.hpp>
// #include <boost/spirit/home/qi/numeric/int.hpp>
// #include <boost/spirit/home/qi/numeric/real.hpp>
#include <boost/spirit/home/qi/operator/sequence.hpp>
#include <boost/spirit/home/qi/operator/alternative.hpp>
// #include <boost/spirit/home/qi/operator/kleene.hpp>
// #include <boost/spirit/home/qi/operator/plus.hpp>
#include <boost/spirit/home/qi/operator/not_predicate.hpp>
#include <boost/spirit/home/qi/auxiliary/eps.hpp>
//#include <boost/spirit/home/qi/parse.hpp>
//#include <boost/spirit/home/qi/parser.hpp>
//#include <boost/spirit/home/qi/string/lit.hpp>

// #include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/home/phoenix/bind/bind_function.hpp>
// #include <boost/spirit/home/phoenix/core/argument.hpp>

#include "redis/redis_grammar_internal.hpp"

template <class Iterator>
void redis_grammar<Iterator>::help_construct_4c() {
    // See comment in redis_grammar.cc about breaking up commands into little blocks.

    // sorted sets
    BEGIN(sortedsets1)
        CMD_N(zadd)
        CMD_1(zcard, string)
        CMD_3(zcount, string, float, float)
        CMD_3(zincrby, string, float, string)
        CMD_N(zinterstore)
        CMD_3(zrange, string, int, int)
        CMD_4(zrange, string, int, int, string)
        CMD_3(zrangebyscore, string, string, string)
        CMD_4(zrangebyscore, string, string, string, string)
        COMMANDS_END

        // sorted sets to be continued in help_construct_5...
}

template class redis_grammar<tcp_conn_t::iterator>;
