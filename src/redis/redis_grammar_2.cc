#include "redis_grammar.hpp"
#include "redis_grammar_internal.hpp"

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
