#include "redis_grammar.hpp"
#include "redis_grammar_internal.hpp"

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
