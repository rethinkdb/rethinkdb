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
void redis_grammar<Iterator>::help_construct_2() {
    // See comment in redis_grammar.cc about breaking up commands into little blocks.

    //KEYS (commands from the 'keys' section of the redis documentation)
    BEGIN(keys1)
        CMD_N(del)
        CMD_1(exists, string)
        CMD_2(expire, string, unsigned)
        CMD_2(expireat, string, unsigned)
        CMD_1(keys, string)
        CMD_2(move, string, string)
        COMMANDS_END
        BEGIN(keys2)
        CMD_1(persist, string)
        CMD_0(randomkey)
        CMD_2(rename, string, string)
        CMD_2(renamenx, string, string)
        CMD_1(ttl, string)
        CMD_1(type, string)
        COMMANDS_END

        //STRINGS
        BEGIN(strings1)
        CMD_2(append, string, string)
        CMD_1(decr, string)
        CMD_2(decrby, string, int)
        CMD_1(get, string)
        CMD_2(getbit, string, unsigned)
        CMD_3(getrange, string, int, int)
        CMD_2(getset, string, string)
        CMD_1(incr, string)
        CMD_2(incrby, string, int)
        COMMANDS_END
        BEGIN(strings2)
        CMD_N(mget)
        CMD_N(mset)
        CMD_N(msetnx)
        CMD_2(set, string, string)
        CMD_3(setbit, string, unsigned, unsigned)
        CMD_3(setex, string, unsigned, string)
        CMD_2(setnx, string, string)
        CMD_3(setrange, string, unsigned, string)
        CMD_1(Strlen, string)
        COMMANDS_END
}


template class redis_grammar<tcp_conn_t::iterator>;
