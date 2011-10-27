#include "redis_grammar.hpp"
#include "redis_grammar_internal.hpp"

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
