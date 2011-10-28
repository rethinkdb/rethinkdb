#include "redis_grammar.hpp"

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

#include "redis_grammar_internal.hpp"

template <class Iterator>
void redis_grammar<Iterator>::help_construct_3() {
    // See comment in redis_grammar.cc about breaking up commands into little blocks.


    //Hashes
    BEGIN(hashes1)
        CMD_N(hdel)
        CMD_2(hexists, string, string)
        CMD_2(hget, string, string)
        CMD_1(hgetall, string)
        CMD_3(hincrby, string, string, int)
        CMD_1(hkeys, string)
        COMMANDS_END
        BEGIN(hashes2)
        CMD_1(hlen, string)
        CMD_N(hmget)
        CMD_N(hmset)
        CMD_3(hset, string, string, string)
        CMD_3(hsetnx, string, string, string)
        CMD_1(hvals, string)
        COMMANDS_END

        //Sets
        BEGIN(sets1)
        CMD_N(sadd)
        CMD_1(scard, string)
        CMD_N(sdiff)
        CMD_N(sdiffstore)
        CMD_N(sinter)
        CMD_N(sinterstore)
        CMD_2(sismember, string, string)
        COMMANDS_END
        BEGIN(sets2)
        CMD_1(smembers, string)
        CMD_3(smove, string, string, string)
        CMD_1(spop, string)
        CMD_1(srandmember, string)
        CMD_N(srem)
        CMD_N(sunion)
        CMD_N(sunionstore)
        COMMANDS_END

}


template class redis_grammar<tcp_conn_t::iterator>;
