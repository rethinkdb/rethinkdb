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
void redis_grammar<Iterator>::help_construct_5() {
    // See comment in redis_grammar.cc about breaking up commands into little blocks.

    // sorted sets, continued...
    BEGIN(sortedsets2)
        CMD_6(zrangebyscore, string, string, string, string, unsigned, unsigned)
        CMD_7(zrangebyscore, string, string, string, string, string, unsigned, unsigned)
        CMD_2(zrank, string, string)
        CMD_N(zrem)
        CMD_3(zremrangebyrank, string, int, int)
        CMD_3(zremrangebyscore, string, float, float)
        CMD_3(zrevrange, string, int, int)
        CMD_4(zrevrange, string, int, int, string)
        COMMANDS_END
        BEGIN(sortedsets3)
        CMD_3(zrevrangebyscore, string, string, string)
        CMD_4(zrevrangebyscore, string, string, string, string)
        CMD_6(zrevrangebyscore, string, string, string, string, unsigned, unsigned)
        CMD_7(zrevrangebyscore, string, string, string, string, string, unsigned, unsigned)
        CMD_2(zrevrank, string, string)
        CMD_2(zscore, string, string)
        CMD_N(zunionstore)
        COMMANDS_END
}

template class redis_grammar<tcp_conn_t::iterator>;
