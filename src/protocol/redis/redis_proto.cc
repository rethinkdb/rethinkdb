#include "protocol/redis/redis_proto.hpp"
#include "protocol/redis/redis.hpp"

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
#define WRITE_0(CNAME)\
        | command(1, std::string(#CNAME))\
        [px::bind(&redis_grammar::execute_write, this, \
         px::construct<redis_protocol_t::write_t>( \
         px::new_<redis_protocol_t::CNAME>()))] \

#define WRITE_1(CNAME, ARG_TYPE_ONE)\
        | command(2, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg)\
        [px::bind(&redis_grammar::execute_write, this, \
         px::construct<redis_protocol_t::write_t>( \
         px::new_<redis_protocol_t::CNAME>(qi::_1)))] \

#define WRITE_2(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO)\
        | command(3, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg)\
        [px::bind(&redis_grammar::execute_write, this, \
         px::construct<redis_protocol_t::write_t>( \
         px::new_<redis_protocol_t::CNAME>(qi::_1, qi::_2)))]

#define WRITE_3(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE)\
        | command(4, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg)\
        [px::bind(&redis_grammar::execute_write, this, \
         px::construct<redis_protocol_t::write_t>( \
         px::new_<redis_protocol_t::CNAME>(qi::_1, qi::_2, qi::_3)))]

#define WRITE_4(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE, ARG_TYPE_FOUR)\
        | command(5, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg >> ARG_TYPE_FOUR##_arg)\
        [px::bind(&redis_grammar::execute_write, this, \
         px::construct<redis_protocol_t::write_t>( \
         px::new_<redis_protocol_t::CNAME>(qi::_1, qi::_2, qi::_3, qi::_4)))]

#define READ__0(CNAME)\
        | command(1, std::string(#CNAME))\
        [px::bind(&redis_grammar::execute_read, this, \
         px::construct<redis_protocol_t::read_t>( \
         px::new_<redis_protocol_t::CNAME>()))] \

#define READ__1(CNAME, ARG_TYPE_ONE)\
        | command(2, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg)\
        [px::bind(&redis_grammar::execute_read, this, \
         px::construct<redis_protocol_t::read_t>( \
         px::new_<redis_protocol_t::CNAME>(qi::_1)))] \

#define READ__2(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO)\
        | command(3, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg)\
        [px::bind(&redis_grammar::execute_read, this, \
         px::construct<redis_protocol_t::read_t>( \
         px::new_<redis_protocol_t::CNAME>(qi::_1, qi::_2)))]

#define READ__3(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE)\
        | command(4, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg)\
        [px::bind(&redis_grammar::execute_read, this, \
         px::construct<redis_protocol_t::read_t>( \
         px::new_<redis_protocol_t::CNAME>(qi::_1, qi::_2, qi::_3)))]

//Some commands take an unsecified number of arguments. These are parsed into a vector of strings. (sorry no other types)
#define WRITE_N(CNAME)\
        | command_n(std::string(#CNAME))\
        [px::bind(&redis_grammar::execute_write, this, \
         px::construct<redis_protocol_t::write_t>( \
         px::new_<redis_protocol_t::CNAME>(qi::_1)))]

#define READ__N(CNAME)\
        | command_n(std::string(#CNAME))\
        [px::bind(&redis_grammar::execute_read, this, \
         px::construct<redis_protocol_t::read_t>( \
         px::new_<redis_protocol_t::CNAME>(qi::_1)))]

#define COMMANDS_END ;

template <typename Iterator>
struct redis_grammar : qi::grammar<Iterator> {
    redis_grammar(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *intface, std::iostream *redis_stream) :
        redis_grammar::base_type(start),
        out_stream(redis_stream),
        namespace_interface(intface),
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
        BEGIN(keys1)
            WRITE_N(del)
            READ__1(exists, string)
            WRITE_2(expire, string, unsigned)
            WRITE_2(expireat, string, unsigned)
            READ__1(keys, string)
            WRITE_2(move, string, string)
        COMMANDS_END
        BEGIN(keys2)
            WRITE_1(persist, string)
            READ__0(randomkey)
            WRITE_2(rename, string, string)
            WRITE_2(renamenx, string, string)
            READ__1(ttl, string)
            READ__1(type, string)
        COMMANDS_END

        //STRINGS
        BEGIN(strings1)
            WRITE_2(append, string, string)
            WRITE_1(decr, string)
            WRITE_2(decrby, string, int)
            READ__1(get, string)
            READ__2(getbit, string, unsigned)
            READ__3(getrange, string, int, int)
            WRITE_2(getset, string, string)
            WRITE_1(incr, string)
            WRITE_2(incrby, string, int)
        COMMANDS_END
        BEGIN(strings2)
            READ__N(mget)
            WRITE_N(mset)
            WRITE_N(msetnx)
            WRITE_2(set, string, string)
            WRITE_3(setbit, string, unsigned, unsigned)
            WRITE_3(setex, string, unsigned, string)
            WRITE_3(setrange, string, unsigned, string)
            READ__1(Strlen, string)
        COMMANDS_END

        //Hashes
        BEGIN(hashes1)
            WRITE_N(hdel)
            READ__2(hexists, string, string)
            READ__2(hget, string, string)
            READ__1(hgetall, string)
            WRITE_3(hincrby, string, string, int)
            READ__1(hkeys, string)
        COMMANDS_END
        BEGIN(hashes2)
            READ__1(hlen, string)
            READ__N(hmget)
            WRITE_N(hmset)
            WRITE_3(hset, string, string, string)
            WRITE_3(hsetnx, string, string, string)
            READ__1(hvals, string)
        COMMANDS_END

        //Sets
        BEGIN(sets1)
           WRITE_N(sadd)
           READ__1(scard, string)
           READ__N(sdiff)
           WRITE_N(sdiffstore)
           READ__N(sinter)
           WRITE_N(sinterstore)
           READ__2(sismember, string, string)
        COMMANDS_END
        BEGIN(sets2)
           READ__1(smembers, string)
           WRITE_3(smove, string, string, string)
           WRITE_1(spop, string)
           READ__1(srandmember, string)
           WRITE_N(srem)
           READ__N(sunion)
           WRITE_N(sunionstore)
        COMMANDS_END

        BEGIN(lists1)
            WRITE_N(blpop)
            WRITE_N(brpop)
            WRITE_N(brpoplpush)
            READ__2(lindex, string, int)
            WRITE_4(linsert, string, string, string, string)
            READ__1(llen, string)
            WRITE_1(lpop, string)
            WRITE_N(lpush)
            WRITE_2(lpushx, string, string)
        COMMANDS_END
        BEGIN(lists2)
            READ__3(lrange, string, int, int)
            WRITE_3(lrem, string, int, string)
            WRITE_3(lset, string, int, string)
            WRITE_3(ltrim, string, int, int)
            WRITE_1(rpop, string)
            WRITE_2(rpoplpush, string, string)
            WRITE_N(rpush)
            WRITE_2(rpushx, string, string)
        COMMANDS_END

        //Because of the aformentioned tiny blocks problem we have to now or the blocks here
        //*sigh* and we were so close to requiring only one line to add a command
        commands = keys1 | keys2 | strings1 | strings2 | hashes1 | hashes2 | sets1 | sets2 | lists1 | lists2;
        start = commands;
    }

    

private:
    std::ostream *out_stream;
    namespace_interface_t<redis_protocol_t> *namespace_interface;
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

    // The call function

    void execute_write(redis_protocol_t::write_t query) {
        redis_protocol_t::write_response_t result =
            namespace_interface->write(query, order_token_t::ignore);
        redis_protocol_t::redis_return_type res = result->get_result();
        boost::apply_visitor(output_visitor(out_conn), res);
    }

    void execute_read(redis_protocol_t::read_t query) {
        redis_protocol_t::read_response_t result =
            namespace_interface->read(query, order_token_t::ignore);
        redis_protocol_t::redis_return_type res = result->get_result();
        boost::apply_visitor(output_visitor(out_conn), res);
    }

    //Output functions
    //These take the results of a redis_interface_t method and send them out on the wire.
    //Handling both input and output here means that this class is solely responsible for
    //translating the redis protocol.

    struct output_visitor : boost::static_visitor<void> {
        output_visitor(tcp_conn_t *conn) : out_conn(conn) {;}
        tcp_conn_t *out_conn;
        
        void operator()(redis_protocol_t::status_result res) const {
            out_conn->write("+", 1);
            out_conn->write(res.msg, strlen(res.msg));
            out_conn->write("\r\n", 2);
        }

        void operator()(redis_protocol_t::error_result res) const {
            out_conn->write("-", 1);
            out_conn->write(res.msg, strlen(res.msg));
            out_conn->write("\r\n", 2);
        }

        void operator()(int res) const {
            char buff[20]; //Max size of a base 10 representation of a 64 bit number
            sprintf(buff, "%d", res);
            out_conn->write(":", 1);
            out_conn->write(buff, strlen(buff));
            out_conn->write("\r\n", 2);
        }

        void bulk_result(std::string &res) const {
            char buff[20];
            sprintf(buff, "%d", (int)res.size());

            out_conn->write("$", 1);
            out_conn->write(buff, strlen(buff));
            out_conn->write("\r\n", 2);
            out_conn->write((char *)(&res.at(0)), res.size());
            out_conn->write("\r\n", 2);

        }

        void operator()(std::string &res) const {
            bulk_result(res);
        }

        void operator()(std::vector<std::string> &res) const {
            char buff[20];
            sprintf(buff, "%d", (int)res.size());

            out_conn->write("*", 1);
            out_conn->write(buff, strlen(buff));
            out_conn->write("\r\n", 2);
            for(std::vector<std::string>::iterator iter = res.begin(); iter != res.end(); ++iter) {
                bulk_result(*iter);
            }
        }
    };

};

// This is obvisously not the right way to do this
namespace_interface_t<redis_protocol_t> *intface;

#include "unittest/unittest_utils.hpp"
#include "server/dummy_namespace_interface.hpp"

//The entry point for the parser. The only requirement for a parser is that it implement this function.
void serve_redis(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store) {
    (void)get_store;
    (void)set_store;

    if(!intface) {
        unittest::temp_file_t db_file("/tmp/redis_test.XXXXXX");

        const int repli_factor = 3;
        std::vector<key_range_t> shards;
        shards.push_back(key_range_t(key_range_t::none,   store_key_t(""),  key_range_t::open, store_key_t("n")));
        shards.push_back(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t("") ));

        dummy_namespace_interface_t<redis_protocol_t>::create(db_file.name(), shards, repli_factor);
        intface = new dummy_namespace_interface_t<redis_protocol_t>(db_file.name(), shards, repli_factor);
    }

    redis_grammar<tcp_conn_t::iterator> redis(conn, intface, NULL);
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
