#include "redis_grammar.hpp"
#include "redis_grammar_internal.hpp"

template <class Iterator>
void redis_grammar<Iterator>::help_construct_6() {
    // See comment in redis_grammar.cc about breaking up commands into little blocks.

    // Pub/sub
    BEGIN(pubsub_ext)
        | command_n(std::string("psubscribe"))[px::bind(&redis_grammar::first_subscribe, this, qi::_1, true)]
        | command(3, std::string("publish")) >> (string_arg >> string_arg)
        [px::bind(&redis_grammar::publish, this, qi::_1, qi::_2)]
        | command_n(std::string("subscribe"))[px::bind(&redis_grammar::first_subscribe, this, qi::_1, false)]
        COMMANDS_END

        BEGIN(pubsub)
        | command_n(std::string("psubscribe"))[px::bind(&redis_grammar::subscribe, this, qi::_1, true)]
        | command_n(std::string("punsubscribe"))[px::bind(&redis_grammar::unsubscribe, this, qi::_1, true)]
        | command_n(std::string("subscribe"))[px::bind(&redis_grammar::subscribe, this, qi::_1, false)]
        | command_n(std::string("unsubscribe"))[px::bind(&redis_grammar::unsubscribe, this, qi::_1, false)]
        | arbitrary_command[px::bind(&redis_grammar::pubsub_error, this)]
        COMMANDS_END
}

template class redis_grammar<tcp_conn_t::iterator>;
