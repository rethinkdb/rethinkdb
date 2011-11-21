#ifndef NO_REDIS
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
void redis_grammar<Iterator>::help_construct_4a() {
    // See comment in redis_grammar.cc about breaking up commands into little blocks.

    // list sets
    BEGIN(lists1)
        CMD_N(blpop)
        CMD_N(brpop)
        CMD_N(brpoplpush)
        CMD_2(lindex, string, int)
        CMD_4(linsert, string, string, string, string)
        CMD_1(llen, string)
        CMD_1(lpop, string)
        CMD_N(lpush)
        CMD_2(lpushx, string, string)
        COMMANDS_END

        // list sets to be continued in help_construct_4b...
}

template class redis_grammar<tcp_conn_t::iterator>;
#endif //#ifndef NO_REDIS
