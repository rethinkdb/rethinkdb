
//These macros greatly reduce the boiler plate needed to add a new command.
//They are structured in such as way as to allow a new command to be included by specifying
//the type signature of the corresponding redis_interface_t method. This expands to a parser
//that will match that command name and parse arguments of the given types and which will then
//call the corresponding redis_interface_t method.

//Begin starts off a command block with a rule guaranteed to fail. This allows us to not have to
//special case the first actual command. All commands can then begin with '|'.
#define BEGIN(RULE) RULE = (!qi::eps)

//Alas! there is no macro overloading or a sufficiently robust variatic macro for our purposes here.
#define CMD_0(CNAME)\
        | command(1, std::string(#CNAME))\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this \
         ))]

#define CMD_1(CNAME, ARG_TYPE_ONE)\
        | command(2, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1 \
         ))]

#define CMD_2(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO)\
        | command(3, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1, qi::_2 \
         ))]

#define CMD_3(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE)\
        | command(4, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1, qi::_2, qi::_3 \
         ))]

#define CMD_4(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE, ARG_TYPE_FOUR)\
        | command(5, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg >> ARG_TYPE_FOUR##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1, qi::_2, qi::_3, qi::_4 \
         ))]

#define CMD_6(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE, ARG_TYPE_FOUR, ARG_TYPE_FIVE, ARG_TYPE_SIX)\
        | command(7, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg >> ARG_TYPE_FOUR##_arg >> ARG_TYPE_FIVE##_arg >> ARG_TYPE_SIX##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6 \
         ))]

#define CMD_7(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE, ARG_TYPE_FOUR, ARG_TYPE_FIVE, ARG_TYPE_SIX, ARG_TYPE_SEVEN)\
        | command(8, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg >> ARG_TYPE_FOUR##_arg >> ARG_TYPE_FIVE##_arg >> ARG_TYPE_SIX##_arg >> ARG_TYPE_SEVEN##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6, qi::_7 \
         ))]


//Some commands take an unsecified number of arguments. These are parsed into a vector of strings. (sorry no other types)
#define CMD_N(CNAME)\
        | command_n(std::string(#CNAME))\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1 \
         ))]

#define COMMANDS_END ;



template <class Iterator>
redis_grammar<Iterator>::redis_grammar(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *intface, pubsub_runtime_t *runtime_)
    : redis_grammar::base_type(start),
      redis_output_writer(conn),
      redis_ext(intface),
      runtime(runtime_) {
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
    float_data %= qi::int_; //TODO figure out how to get the real number parser working
    float_arg = arg_bytes >> float_data[qi::_val = qi::_1] >> eol;

    command = args(qi::_r1) >> cname(qi::_r2);
    command_n = args_n[qi::_a = qi::_1] >> cname(qi::_r1) >> (qi::repeat(qi::_a - 1)[string_arg])[qi::_val = qi::_1];
    arbitrary_command = args_n[qi::_a = qi::_1] >> qi::repeat(qi::_a)[string_arg];

    //Commands

    //Why break up the commands into a bunch of little blocks? If we have too many commands in one block we
    //get a symbol error in the assembler. Turns out that the mangled name of a rule is proportional to
    //the number of alternatives. Too many and we exceed the maximum length of a symbol name, boo.

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

        //Hashes
        BEGIN(hashes1)
        CMD_N(hdel)
        CMD_2(hexists, string, string)
        CMD_2(hget, string, string)
        CMD_1(hgetall, string)
        CMD_3(hincrby, string, string, int)
        CMD_1(hkeys, string)
        COMMANDS_END
        BEGIN(hashes2)
        CMD_1(hlen, string)
        CMD_N(hmget)
        CMD_N(hmset)
        CMD_3(hset, string, string, string)
        CMD_3(hsetnx, string, string, string)
        CMD_1(hvals, string)
        COMMANDS_END

        //Sets
        BEGIN(sets1)
        CMD_N(sadd)
        CMD_1(scard, string)
        CMD_N(sdiff)
        CMD_N(sdiffstore)
        CMD_N(sinter)
        CMD_N(sinterstore)
        CMD_2(sismember, string, string)
        COMMANDS_END
        BEGIN(sets2)
        CMD_1(smembers, string)
        CMD_3(smove, string, string, string)
        CMD_1(spop, string)
        CMD_1(srandmember, string)
        CMD_N(srem)
        CMD_N(sunion)
        CMD_N(sunionstore)
        COMMANDS_END

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

        BEGIN(sortedsets1)
        CMD_N(zadd)
        CMD_1(zcard, string)
        CMD_3(zcount, string, float, float)
        CMD_3(zincrby, string, float, string)
        CMD_N(zinterstore)
        CMD_3(zrange, string, int, int)
        CMD_4(zrange, string, int, int, string)
        CMD_3(zrangebyscore, string, string, string)
        CMD_4(zrangebyscore, string, string, string, string)
        COMMANDS_END
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

#undef BEGIN
#undef CMD_0
#undef CMD_1
#undef CMD_2
#undef CMD_3
#undef CMD_4
#undef CMD_5
#undef CMD_6
#undef CMD_7
#undef CMD_N

