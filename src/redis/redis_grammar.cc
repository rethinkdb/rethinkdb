#include "redis/redis_grammar.hpp"

// #include <boost/spirit/home/qi/action/action.hpp>
// #include <boost/spirit/home/qi/char/char.hpp>
// #include <boost/spirit/home/qi/directive/no_case.hpp>
// #include <boost/spirit/home/qi/directive/repeat.hpp>
// #include <boost/spirit/home/qi/directive/as.hpp>
// #include <boost/spirit/home/qi/nonterminal/rule.hpp>
// #include <boost/spirit/home/qi/nonterminal/grammar.hpp>
// #include <boost/spirit/home/qi/numeric/uint.hpp>
// #include <boost/spirit/home/qi/numeric/int.hpp>
// #include <boost/spirit/home/qi/numeric/real.hpp>
// #include <boost/spirit/home/qi/operator/sequence.hpp>
#include <boost/spirit/home/qi/operator/alternative.hpp>
// #include <boost/spirit/home/qi/operator/kleene.hpp>
// #include <boost/spirit/home/qi/operator/plus.hpp>
// #include <boost/spirit/home/qi/operator/not_predicate.hpp>
// #include <boost/spirit/home/qi/auxiliary/eps.hpp>
#include <boost/spirit/home/qi/parse.hpp>
// #include <boost/spirit/home/qi/parser.hpp>
// #include <boost/spirit/home/qi/string/lit.hpp>

// #include <boost/spirit/include/support_istream_iterator.hpp>
// #include <boost/spirit/include/phoenix.hpp>
// #include <boost/spirit/home/phoenix/bind/bind_function.hpp>
// #include <boost/spirit/home/phoenix/core/argument.hpp>

#include "redis/redis_grammar_internal.hpp"


template <class Iterator>
redis_grammar<Iterator>::redis_grammar(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *intface, pubsub_runtime_t *runtime_)
    : redis_grammar::base_type(start),
      redis_output_writer(conn),
      redis_ext(intface),
      runtime(runtime_) {

    help_construct_1();

    //Commands

    //Why break up the commands into a bunch of little blocks? If we have too many commands in one block we
    //get a symbol error in the assembler. Turns out that the mangled name of a rule is proportional to
    //the number of alternatives. Too many and we exceed the maximum length of a symbol name, boo.
    help_construct_2();
    help_construct_3();
    help_construct_4a();
    help_construct_4b();
    help_construct_4c();
    help_construct_5();
    help_construct_6();



    // TODO better than this
    srand(time(NULL));
    connection_id = rand();
    subscribed_channels = 0;

    //Because of the aformentioned tiny blocks problem we have to now or the blocks here
    //*sigh* and we were so close to requiring only one line to add a command
    commands = keys1 | keys2 | strings1 | strings2 | hashes1 | hashes2 | sets1 |
        sets2 | lists1 | lists2 | sortedsets1 | sortedsets2 | sortedsets3;
    start = commands | pubsub_ext;
}


template <class Iterator>
void redis_grammar<Iterator>::first_subscribe(std::vector<std::string> &channels, bool patterned) {
    subscribe(channels, patterned);

    // After subscription we only accept pub/sub commands until unsubscribe
    parse_pubsub();
}

template <class Iterator>
void redis_grammar<Iterator>::subscribe(std::vector<std::string> &channels, bool patterned) {
    for(std::vector<std::string>::iterator iter = channels.begin(); iter != channels.end(); iter++) {
        // Add our subscription
        if(patterned) subscribed_channels += runtime->subscribe_pattern(*iter, connection_id, this);
        else          subscribed_channels += runtime->subscribe(*iter, connection_id, this);

        // Output the successful subscription message
        std::vector<std::string> subscription_message;
        subscription_message.push_back(std::string("subscribe"));
        subscription_message.push_back(*iter);
        subscription_message.push_back(boost::lexical_cast<std::string>(subscribed_channels));
        output_response(redis_protocol_t::redis_return_type(subscription_message));
    }
}

template <class Iterator>
void redis_grammar<Iterator>::unsubscribe(std::vector<std::string> &channels, bool patterned) {
    for(std::vector<std::string>::iterator iter = channels.begin(); iter != channels.end(); iter++) {
        // Add our subscription
        if(patterned) subscribed_channels -= runtime->unsubscribe_pattern(*iter, connection_id);
        else          subscribed_channels -= runtime->unsubscribe(*iter, connection_id);

        // Output the sucessful unsubscription message
        std::vector<std::string> unsubscription_message;
        unsubscription_message.push_back(std::string("unsubscribe"));
        unsubscription_message.push_back(*iter);
        unsubscription_message.push_back(boost::lexical_cast<std::string>(subscribed_channels));
        output_response(redis_protocol_t::redis_return_type(unsubscription_message));
    }
}

template <class Iterator>
void redis_grammar<Iterator>::publish(std::string &channel, std::string &message) {
    // This behaves like a normal command. No special states associated here
    int clients_recieved = runtime->publish(channel, message);
    output_response(redis_protocol_t::redis_return_type(clients_recieved));
}


template <class Iterator>
void redis_grammar<Iterator>::parse_pubsub() {
    // loop on parse. Parse just pub/sub commands

    while(true) {
        // Clear data read from the connection
        const_charslice res = out_conn->peek();
        size_t len = res.end - res.beg;
        out_conn->pop(len);

        // parse pub/sub commands
        qi::parse(out_conn->begin(), out_conn->end(), pubsub);

        if(subscribed_channels == 0) {
            // We've unsubscribed from all our channels. Leave pub/sub mode.
            break;
        }
    }
}

template <class Iterator>
void redis_grammar<Iterator>::pubsub_error() {
    output_response(redis_protocol_t::error_result(
        "ERR only (P)SUBSCRIBE / (P)UNSUBSCRIBE / QUIT allowed in this contex"
    ));
}


template class redis_grammar<tcp_conn_t::iterator>;
