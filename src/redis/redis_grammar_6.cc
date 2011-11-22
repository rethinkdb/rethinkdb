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
#endif //#ifndef NO_REDIS
