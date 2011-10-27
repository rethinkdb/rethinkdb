#include "redis_grammar.hpp"
#include "redis_grammar_internal.hpp"

template <class Iterator>
void redis_grammar<Iterator>::help_construct_4b() {
    // See comment in redis_grammar.cc about breaking up commands into little blocks.

    // list sets continued from help_construct_4a...
    BEGIN(lists2)
        CMD_3(lrange, string, int, int)
        CMD_3(lrem, string, int, string)
        CMD_3(lset, string, int, string)
        CMD_3(ltrim, string, int, int)
        CMD_1(rpop, string)
        CMD_2(rpoplpush, string, string)
        CMD_N(rpush)
        CMD_2(rpushx, string, string)
        COMMANDS_END
}

template class redis_grammar<tcp_conn_t::iterator>;
