#include "redis_grammar.hpp"
#include "redis_grammar_internal.hpp"

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
