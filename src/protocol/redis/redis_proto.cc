#include "protocol/redis/redis_proto.hpp"
#include "protocol/redis/redis.hpp"

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

//These macros greatly reduce the boiler plate needed to add a new command.
//They are structured in such as way as to allow a new command to be included by specifying
//the type signature of the corresponding redis_interface_t method. This expands to a parser
//that will match that command name and parse arguments of the given types and which will then
//call the corresponding redis_interface_t method.

//Begin starts off a command block with a rule guaranteed to fail. This allows us to not have to
//special case the first actual command. All commands can then begin with '|'.
#define BEGIN(RULE) RULE = (!qi::eps)

//Alas! there is no macro overloading or a sufficiently robust variatic macro for our purposes here.
#define COMMAND_0(OUTPUT, CNAME)\
        | command(1, std::string(#CNAME))\
        [px::bind(&redis_grammar::output_##OUTPUT##_result, this, px::bind(&redis_interface_t::CNAME, this))]
#define COMMAND_1(OUTPUT, CNAME, ARG_TYPE_ONE)\
        | command(2, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg)\
        [px::bind(&redis_grammar::output_##OUTPUT##_result, this, px::bind(&redis_interface_t::CNAME, this, qi::_1))]
#define COMMAND_2(OUTPUT, CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO)\
        | command(3, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg)\
        [px::bind(&redis_grammar::output_##OUTPUT##_result, this, px::bind(&redis_interface_t::CNAME, this, qi::_1, qi::_2))]
#define COMMAND_3(OUTPUT, CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE)\
        | command(4, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg)\
        [px::bind(&redis_grammar::output_##OUTPUT##_result, this, px::bind(&redis_interface_t::CNAME, this, qi::_1, qi::_2, qi::_3))]

//Some commands take an unsecified number of arguments. These are parsed into a vector of strings. (sorry no other types)
#define COMMAND_N(OUTPUT, CNAME)\
        | command_n(std::string(#CNAME))\
        [px::bind(&redis_grammar::output_##OUTPUT##_result, this, px::bind(&redis_interface_t::CNAME, this, qi::_1))]

#define COMMANDS_END ;

template <typename Iterator>
struct redis_grammar : qi::grammar<Iterator>, redis_interface_t {
    redis_grammar(tcp_conn_t *conn, std::iostream *redis_stream) :
        redis_grammar::base_type(start),
        redis_interface_t(),
        out_stream(redis_stream),
        out_conn(conn)
    {
        eol = qi::lit("\r\n");
        args = '*' >> qi::uint_(qi::_r1) >> eol;
        args_n %= '*' >> qi::uint_ >> eol;
        arg_bytes %= '$' >> qi::uint_ >> eol;
        cname = '$' >> qi::uint_(px::size(qi::_r1)) >> eol
                >> qi::no_case[qi::string(qi::_r1)] >> eol;
        string_data %= qi::as_string[qi::repeat(qi::_r1)[qi::char_]];
        string_arg = arg_bytes[qi::_a = qi::_1] >> string_data(qi::_a)[qi::_val = qi::_1] >> eol;
        unsigned_data %= qi::uint_;
        unsigned_arg = arg_bytes >> unsigned_data[qi::_val = qi::_1] >> eol;
        int_data %= qi::int_;
        int_arg = arg_bytes >> int_data[qi::_val = qi::_1] >> eol;

        command = args(qi::_r1) >> cname(qi::_r2);
        command_n = args_n[qi::_a = qi::_1] >> cname(qi::_r1) >> (qi::repeat(qi::_a - 1)[string_arg])[qi::_val = qi::_1];

        //Commands

        //Why break up the commands into a bunch of little blocks? If we have too many commands in one block we
        //get a symbol error in the assembler. Turns out that the mangled name of a rule is proportional to
        //the number of alternatives. Too many and we exceed the maximum length of a symbol name, boo.

        //KEYS (commands from the 'keys' section of the redis documentation)
        BEGIN(keys)
            COMMAND_N(integer, del)
            COMMAND_1(integer, exists, string)
            COMMAND_2(integer, expire, string, unsigned)
            COMMAND_2(integer, expireat, string, unsigned)
            COMMAND_1(multi_bulk, keys, string)
            COMMAND_2(integer, move, string, string)
            COMMAND_1(integer, persist, string)
            COMMAND_0(bulk, randomkey)
            COMMAND_2(status, rename, string, string)
            COMMAND_2(integer, renamenx, string, string)
            COMMAND_1(integer, ttl, string)
            COMMAND_1(status, type, string)
        COMMANDS_END

        //STRINGS
        BEGIN(strings1)
            COMMAND_2(integer, append, string, string)
            COMMAND_1(integer, decr, string)
            COMMAND_2(integer, decrby, string, int)
            COMMAND_1(bulk, get, string)
            COMMAND_2(integer, getbit, string, unsigned)
            COMMAND_3(bulk, getrange, string, int, int)
            COMMAND_2(bulk, getset, string, string)
            COMMAND_1(integer, incr, string)
            COMMAND_2(integer, incrby, string, int)
        COMMANDS_END
        BEGIN(strings2)
            COMMAND_N(multi_bulk, mget)
            COMMAND_N(status, mset)
            COMMAND_N(integer, msetnx)
            COMMAND_2(status, set, string, string)
            COMMAND_3(integer, setbit, string, unsigned, unsigned)
            COMMAND_3(status, setex, string, unsigned, string)
            COMMAND_3(integer, setrange, string, unsigned, string)
            COMMAND_1(integer, Strlen, string)
        COMMANDS_END

        //Hashes
        BEGIN(hashes)
            COMMAND_N(integer, hdel)
            COMMAND_2(integer, hexists, string, string)
            COMMAND_2(bulk, hget, string, string)
            COMMAND_1(multi_bulk, hgetall, string)
            COMMAND_3(integer, hincrby, string, string, int)
            COMMAND_1(multi_bulk, hkeys, string)
            COMMAND_1(integer, hlen, string)
            COMMAND_N(multi_bulk, hmget)
            COMMAND_N(status, hmset)
            COMMAND_3(integer, hset, string, string, string)
            COMMAND_3(integer, hsetnx, string, string, string)
            COMMAND_1(multi_bulk, hvals, string)
        COMMANDS_END

        //Because of the aformentioned tiny blocks problem we have to now or the blocks here
        //*sigh* and we were so close to requiring only one line to add a command
        commands = keys | strings1 | strings2 | hashes;
        start = commands;
    }

    

private:
    std::ostream *out_stream;
    tcp_conn_t *out_conn;

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

    qi::rule<Iterator, void(unsigned, std::string)> command;
    qi::rule<Iterator, std::vector<std::string>(std::string), qi::locals<unsigned> > command_n;

    //Command blocks
    qi::rule<Iterator> commands;
    qi::rule<Iterator> keys;
    qi::rule<Iterator> strings1;
    qi::rule<Iterator> strings2;
    qi::rule<Iterator> hashes;
    qi::rule<Iterator> start;

    //Output functions
    //These take the results of a redis_interface_t method and send them out on the wire.
    //Handling both input and output here means that this class is solely responsible for
    //translating the redis protocol.
    
    void output_status_result(status_result res) {
        if(res->status == OK) out_conn->write("+", 1);
        else                  out_conn->write("-", 1);
        out_conn->write(res->msg, strlen(res->msg));
        out_conn->write("\r\n", 2);
    }
    
    void output_integer_result(integer_result res) {
        if(res.which() == 1) {
            //Error
            output_status_result(boost::get<status_result>(res));
        } else {
            char buff[20]; //Max size of a base 10 representation of a 64 bit number
            const unsigned value = boost::get<const unsigned>(res);
            sprintf(buff, "%d", value);
            out_conn->write(":", 1);
            out_conn->write(buff, strlen(buff));
            out_conn->write("\r\n", 2);
        }
    }

    //Utility used by output bulk result and multi_bulk result
    void output_result_vector(const std::string &one) {
        char buff[20];
        sprintf(buff, "%d", (int)one.size());

        out_conn->write("$", 1);
        out_conn->write(buff, strlen(buff));
        out_conn->write("\r\n", 2);
        out_conn->write((char *)(&one.at(0)), one.size());
        out_conn->write("\r\n", 2);
    }

    void output_nil() {
        out_conn->write("$-1\r\n", 5);
    }

    void output_bulk_result(bulk_result res) {
        if(res.which() == 1) {
            //Error
            output_status_result(boost::get<status_result>(res));
        } else {
            boost::shared_ptr<std::string> result = boost::get<boost::shared_ptr<std::string> >(res);
            if(result == NULL) {
                output_nil();
            } else {
                output_result_vector(*result);
            }
        }
    }

    void output_multi_bulk_result(multi_bulk_result res) {
        if(res.which() == 1) {
            //Error
            output_status_result(boost::get<status_result>(res));
        } else {
            boost::shared_ptr<std::vector<std::string> > result = boost::get<boost::shared_ptr<std::vector<std::string> > >(res);
            char buff[20];
            sprintf(buff, "%d", (int)result->size());

            out_conn->write("*", 1);
            out_conn->write(buff, strlen(buff));
            out_conn->write("\r\n", 2);
            for(std::vector<std::string>::iterator iter = result->begin(); iter != result->end(); ++iter) {
                output_result_vector(*iter);
            }
        }
    }

};

//The entry point for the parser. The only requirement for a parser is that it implement this function.
void serve_redis(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store) {
    (void)get_store;
    (void)set_store;
    redis_grammar<tcp_conn_t::iterator> redis(conn, NULL);
    try {
        while(true) {
            qi::parse(conn->begin(), conn->end(), redis);
            //if(!r) {
                const_charslice res = conn->peek();
                size_t len = res.end - res.beg;
                conn->pop(len);
            //}
        }
    } catch(tcp_conn_t::read_closed_exc_t) {}
}

/*
void serve_redis(std::iostream &redis_stream, get_store_t *get_store, set_store_interface_t *set_store) {
    redis_stream.unsetf(std::ios::skipws);
    redis_grammar<boost::spirit::basic_istream_iterator<char> > redis(NULL, &redis_stream, get_store, set_store);
    boost::spirit::istream_iterator begin(redis_stream);
    boost::spirit::istream_iterator end;
    while(true) {
        bool r = qi::parse(begin, end, redis);
        if(!r) std::cout << "Bad parse, will I have to now extract the bad data?" << std::endl;
        while(begin != end) begin++;
    }

}
*/

/*
void serve_redis(std::iostream &redis_stream, get_store_t *get_store, set_store_interface_t *set_store) {
    qi::rule<boost::spirit::basic_istream_iterator<char> > junk = *(qi::char_ >> "\r\n")[printj];

    redis_grammar<boost::spirit::basic_istream_iterator<char> > redis(NULL, &redis_stream, get_store, set_store);
    //qi::detail::match_manip<redis_grammar<boost::spirit::basic_istream_iterator<char, std::char_traits<char> > >,
    //    mpl_::bool_<false>, mpl_::bool_<false>, boost::spirit::unused_type, const boost::spirit::unused_type>
    //    match = qi::match(redis);
    qi::rule<boost::spirit::basic_istream_iterator<char> > valid = *(qi::lit('*')) >> "\r\n";//'*' << qi::uint_ << "\r\n";
    while(redis_stream.good()) {
                        //TODO construct the manipulator outside of the loop?
        //redis_stream >> qi::match(redis);
        redis_stream >> qi::match(valid);
        if(redis_stream.fail()) {
            std::cout << "Bad parse, will I have to now extract the bad data?" << std::endl;
            //redis_stream >> qi::match(junk);
            redis_stream.clear();
        }
    }
}
*/
