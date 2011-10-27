#ifndef REDIS_REDIS_GRAMMAR_HPP_
#define REDIS_REDIS_GRAMMAR_HPP_

#include "redis/redis_proto.hpp"
#include "redis/redis.hpp"
#include "redis/redis_ext.hpp"
#include "redis/pubsub.hpp"

#include <boost/variant.hpp>

//Spirit QI
//Note this result could have been achieved with <boost/spirit/include/qi.hpp> but I expanded
//it out in the hopes that this would reduce compile time, it did by about half a second
#include <boost/spirit/home/qi/action/action.hpp>
#include <boost/spirit/home/qi/char/char.hpp>
#include <boost/spirit/home/qi/directive/no_case.hpp>
#include <boost/spirit/home/qi/directive/repeat.hpp>
#include <boost/spirit/home/qi/directive/as.hpp>
#include <boost/spirit/home/qi/nonterminal/rule.hpp>
#include <boost/spirit/home/qi/nonterminal/grammar.hpp>
#include <boost/spirit/home/qi/numeric/uint.hpp>
#include <boost/spirit/home/qi/numeric/int.hpp>
#include <boost/spirit/home/qi/numeric/real.hpp>
#include <boost/spirit/home/qi/operator/sequence.hpp>
#include <boost/spirit/home/qi/operator/alternative.hpp>
#include <boost/spirit/home/qi/operator/kleene.hpp>
#include <boost/spirit/home/qi/operator/plus.hpp>
#include <boost/spirit/home/qi/operator/not_predicate.hpp>
#include <boost/spirit/home/qi/auxiliary/eps.hpp>
#include <boost/spirit/home/qi/parse.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/string/lit.hpp>

#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/home/phoenix/bind/bind_function.hpp> 
#include <boost/spirit/home/phoenix/core/argument.hpp>

//These aliases greatly reduce typeing. I avoided "using namespce qi" to avoid any
//confusion in the code as to where some of these weird spirit symbols came from
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace px = boost::phoenix;

template <typename Iterator>
struct redis_grammar : qi::grammar<Iterator>, redis_output_writer, redis_ext {
    redis_grammar(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *intface, pubsub_runtime_t *runtime_);

private:
    pubsub_runtime_t *runtime;

    //Support rules
    qi::rule<Iterator> eol;
    qi::rule<Iterator, unsigned(unsigned)> args;
    qi::rule<Iterator, unsigned()> args_n;
    qi::rule<Iterator, void(std::string)> cname;
    qi::rule<Iterator, unsigned()> arg_bytes;
    qi::rule<Iterator, std::string(unsigned)> string_data;
    qi::rule<Iterator, std::string(), qi::locals<unsigned> > string_arg;
    qi::rule<Iterator, unsigned()> unsigned_data;
    qi::rule<Iterator, unsigned()> unsigned_arg;
    qi::rule<Iterator, int()> int_data;
    qi::rule<Iterator, int()> int_arg;
    qi::rule<Iterator, float()> float_data;
    qi::rule<Iterator, float()> float_arg;

    qi::rule<Iterator, void(unsigned, std::string)> command;
    qi::rule<Iterator, std::vector<std::string>(std::string), qi::locals<unsigned> > command_n;
    qi::rule<Iterator, void(), qi::locals<unsigned> > arbitrary_command;

    //Command blocks
    qi::rule<Iterator> commands;
    qi::rule<Iterator> keys1;
    qi::rule<Iterator> keys2;
    qi::rule<Iterator> strings1;
    qi::rule<Iterator> strings2;
    qi::rule<Iterator> hashes1;
    qi::rule<Iterator> hashes2;
    qi::rule<Iterator> start;
    qi::rule<Iterator> sets1;
    qi::rule<Iterator> sets2;
    qi::rule<Iterator> lists1;
    qi::rule<Iterator> lists2;
    qi::rule<Iterator> sortedsets1;
    qi::rule<Iterator> sortedsets2;
    qi::rule<Iterator> sortedsets3;
    qi::rule<Iterator> pubsub_ext;
    qi::rule<Iterator> pubsub;

    
    // Pub/sub support

    // Randomly generated (and so hopefully unique) id for this connection. Used to identify this connection
    // when subscribing and unscribing from channels
    uint64_t connection_id;
    uint64_t subscribed_channels;

    void first_subscribe(std::vector<std::string> &channels, bool patterned);

    void subscribe(std::vector<std::string> &channels, bool patterned);

    void unsubscribe(std::vector<std::string> &channels, bool patterned);

    void publish(std::string &channel, std::string &message);

    void parse_pubsub();

    void pubsub_error();

};

#include "redis/redis_grammar.tcc"


#endif  // REDIS_REDIS_GRAMMAR_HPP_
