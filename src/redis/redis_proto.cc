#include "errors.hpp"
#include <boost/algorithm/string.hpp>

#include "arch/io/network.hpp"
#include "redis/redis_proto.hpp"

class RedisTypeError : public ParseError {
public:
    virtual const char *what() const throw() {
        return "Wrong number of arguments applied to Redis command";
    }
} typeException;

class phantom {};

redis_protocol_t::redis_return_type command_t::execute(redis_ext *extApi, std::vector<std::string> &args) {
    switch(args.size()) {
    case 0:
        return (*this)(extApi);
    case 1:
        return (*this)(extApi, args[0]);
    case 2:
        return (*this)(extApi, args[0], args[1]);
    case 3:
        return (*this)(extApi, args[0], args[1], args[2]);
    case 4:
        return (*this)(extApi, args[0], args[1], args[2], args[3]);
    default:
        unreachable();
    }
}

redis_protocol_t::redis_return_type command_t::operator()(UNUSED redis_ext *extApi) {
    throw typeException;
}

redis_protocol_t::redis_return_type command_t::operator()(UNUSED redis_ext *extApi, UNUSED std::string& arg1_string) {
    throw typeException;
}

redis_protocol_t::redis_return_type command_t::operator()(UNUSED redis_ext *extApi, UNUSED std::string& arg1_string, UNUSED std::string& arg2_string) {
    throw typeException;
}

redis_protocol_t::redis_return_type command_t::operator()(UNUSED redis_ext *extApi, UNUSED std::string& arg1_string, UNUSED std::string& arg2_string, UNUSED std::string& arg3_string) {
    throw typeException;
}

redis_protocol_t::redis_return_type command_t::operator()(UNUSED redis_ext *extApi, UNUSED std::string& arg1_string, UNUSED std::string& arg2_string, UNUSED std::string& arg3_string, UNUSED std::string &arg4_string) {
    throw typeException;
}

template <typename arg1T = phantom, typename arg2T = phantom, typename arg3T = phantom, typename arg4T = phantom>
class cmd : public command_t {
private:
    typedef redis_protocol_t::redis_return_type (redis_ext::*fptr)(arg1T, arg2T, arg3T, arg4T);
    fptr action;

public:
    explicit cmd(fptr action_) : action(action_) {}

    redis_protocol_t::redis_return_type operator()(redis_ext *extApi, std::string& arg1_string, std::string& arg2_string, std::string& arg3_string, std::string &arg4_string) {
        try {
            arg1T arg1 = boost::lexical_cast<arg1T>(arg1_string);
            arg2T arg2 = boost::lexical_cast<arg2T>(arg2_string);
            arg3T arg3 = boost::lexical_cast<arg3T>(arg3_string);
            arg4T arg4 = boost::lexical_cast<arg4T>(arg4_string);
            return (extApi->*(static_cast<fptr>(action)))(arg1, arg2, arg3, arg4);
        } catch (boost::bad_lexical_cast) {
            throw typeException;
        }
    }

    using command_t::operator();
};


template <typename arg1T, typename arg2T, typename arg3T>
class cmd<arg1T, arg2T, arg3T, phantom> : public command_t {
private:
    typedef redis_protocol_t::redis_return_type (redis_ext::*fptr)(arg1T, arg2T, arg3T);
    fptr action;

public:
    explicit cmd(fptr action_) : action(action_) {}

    redis_protocol_t::redis_return_type operator()(redis_ext *extApi, std::string& arg1_string, std::string& arg2_string, std::string& arg3_string) {
        try {
            arg1T arg1 = boost::lexical_cast<arg1T>(arg1_string);
            arg2T arg2 = boost::lexical_cast<arg2T>(arg2_string);
            arg3T arg3 = boost::lexical_cast<arg3T>(arg3_string);
            return (extApi->*(static_cast<fptr>(action)))(arg1, arg2, arg3);
        } catch (boost::bad_lexical_cast) {
            throw typeException;
        }
    }

    using command_t::operator();
};

template <typename arg1T, typename arg2T>
class cmd<arg1T, arg2T, phantom, phantom> : public command_t {
private:
    typedef redis_protocol_t::redis_return_type (redis_ext::*fptr)(arg1T, arg2T);
    fptr action;

public:
    explicit cmd(fptr action_) : action(action_) {}

    redis_protocol_t::redis_return_type operator()(redis_ext *extApi, std::string& arg1_string, std::string& arg2_string) {
        try {
            arg1T arg1 = boost::lexical_cast<arg1T>(arg1_string);
            arg2T arg2 = boost::lexical_cast<arg2T>(arg2_string);
            return (extApi->*(static_cast<fptr>(action)))(arg1, arg2);
        } catch (boost::bad_lexical_cast) {
            throw typeException;
        }
    }

    using command_t::operator();
};

template <typename arg1T>
class cmd<arg1T, phantom, phantom, phantom> : public command_t  {
private:
    typedef redis_protocol_t::redis_return_type (redis_ext::*fptr)(arg1T);
    fptr action;
public:
    explicit cmd(fptr action_) : action(action_) {}

    virtual redis_protocol_t::redis_return_type operator()(redis_ext *extApi, std::string& arg1_string) {
        try {
            arg1T arg1 = boost::lexical_cast<arg1T>(arg1_string);
            return (extApi->*(static_cast<fptr>(action)))(arg1);
        } catch (boost::bad_lexical_cast) {
            throw typeException;
        }
    }

    using command_t::operator();
};

template <>
class cmd<phantom, phantom, phantom, phantom> : public command_t  {
private:
    typedef redis_protocol_t::redis_return_type (redis_ext::*fptr)();
    fptr action;
public:
    explicit cmd(fptr action_) : action(action_) {}

    virtual redis_protocol_t::redis_return_type operator()(redis_ext *extApi) {
        return (extApi->*(static_cast<fptr>(action)))();
    }

    using command_t::operator();
};

template <>
class cmd<std::vector<std::string>, phantom, phantom, phantom> : public command_t {
private:
    typedef redis_protocol_t::redis_return_type (redis_ext::*fptr)(std::vector<std::string>);
    fptr action;
public:
    explicit cmd(fptr action_) : action(action_) {}

    virtual redis_protocol_t::redis_return_type execute(redis_ext *extApi, std::vector<std::string> &args) {
        return (extApi->*(static_cast<fptr>(action)))(args);
    }
};

class pubsub_subscribe_cmd : public command_t {
private:
    RedisParser *parser;
    bool patterned;
public:
    pubsub_subscribe_cmd(RedisParser *parser_, bool patterned_) : parser(parser_), patterned(patterned_) {}

    virtual redis_protocol_t::redis_return_type execute(UNUSED redis_ext *extApi, std::vector<std::string> &args) {
        parser->first_subscribe(args, patterned);
        return redis_protocol_t::redis_return_type(redis_protocol_t::nil_result());
    }
};

class pubsub_publish_cmd : public command_t {
private:
    RedisParser *parser;
public:
    explicit pubsub_publish_cmd(RedisParser *parser_) : parser(parser_) {}

    virtual redis_protocol_t::redis_return_type operator()(UNUSED redis_ext *extApi, std::string &channel, std::string &message) {
        return parser->publish(channel, message);
    }

    //Bring in the other definitions of opertor()
    using command_t::operator();
};

#define CMD(NAME, ...) commands[#NAME] = new cmd<__VA_ARGS__>(&redis_ext::NAME);

RedisParser::RedisParser(tcp_conn_t *conn_, namespace_interface_t<redis_protocol_t> *intf, pubsub_runtime_t *runtime_) :
     redis_ext(intf),
     redis_output_writer(conn_),
     runtime(runtime_),
     conn(conn_)
{
    //TODO better than this

    // Originally this line had rand().  I changed this to
    // randint(RAND_MAX) because rand() is not thread safe.
    connection_id = randint(RAND_MAX);
    subscribed_channels = 0;

    // All redis commands declared here

    // Keys commands
    CMD(del, std::vector<std::string>)
    CMD(exists, std::string)
    CMD(expire, std::string, unsigned)
    CMD(expireat, std::string, unsigned)
    CMD(keys, std::string)
    CMD(move, std::string, std::string)
    CMD(persist, std::string)
    CMD(randomkey)
    CMD(rename, std::string, std::string)
    CMD(renamenx, std::string, std::string)
    CMD(ttl, std::string)
    CMD(type, std::string)

    // Strings commands
    CMD(append, std::string, std::string)
    CMD(decr, std::string)
    CMD(decrby, std::string, int)
    CMD(get, std::string)
    CMD(getbit, std::string, unsigned)
    CMD(getrange, std::string, int, int)
    CMD(getset, std::string, std::string)
    CMD(incr, std::string)
    CMD(incrby, std::string, int)
    CMD(mget, std::vector<std::string>)
    CMD(mset, std::vector<std::string>)
    CMD(msetnx, std::vector<std::string>)
    CMD(set, std::string, std::string)
    CMD(setbit, std::string, unsigned, unsigned)
    CMD(setex, std::string, unsigned, std::string)
    CMD(setnx, std::string, std::string)
    CMD(setrange, std::string, unsigned, std::string)
    CMD(Strlen, std::string)
    commands["strlen"] = new cmd<std::string>(&redis_ext::Strlen);

    // Hashes
    CMD(hdel, std::vector<std::string>)
    CMD(hexists, std::string, std::string)
    CMD(hget, std::string, std::string)
    CMD(hgetall, std::string)
    CMD(hincrby, std::string, std::string, int)
    CMD(hkeys, std::string)
    CMD(hlen, std::string)
    CMD(hmget, std::vector<std::string>)
    CMD(hmset, std::vector<std::string>)
    CMD(hset, std::string, std::string, std::string)
    CMD(hsetnx, std::string, std::string, std::string)
    CMD(hvals, std::string)

    // Sets
    CMD(sadd, std::vector<std::string>)
    CMD(scard, std::string)
    CMD(sdiff, std::vector<std::string>)
    CMD(sdiffstore, std::vector<std::string>)
    CMD(sinter, std::vector<std::string>)
    CMD(sinterstore, std::vector<std::string>)
    CMD(sismember, std::string, std::string)
    CMD(smembers, std::string)
    CMD(smove, std::string, std::string, std::string)
    CMD(spop, std::string)
    CMD(srandmember, std::string)
    CMD(srem, std::vector<std::string>)
    CMD(sunion, std::vector<std::string>)
    CMD(sunionstore, std::vector<std::string>)

    // Lists
    CMD(blpop, std::vector<std::string>)
    CMD(brpop, std::vector<std::string>)
    CMD(brpoplpush, std::vector<std::string>)
    CMD(lindex, std::string, int)
    CMD(linsert, std::string, std::string, std::string, std::string)
    CMD(llen, std::string)
    CMD(lpop, std::string)
    CMD(lpush, std::vector<std::string>)
    CMD(lpushx, std::string, std::string)
    CMD(lrange, std::string, int, int)
    CMD(lrem, std::string, int, std::string)
    CMD(lset, std::string, int, std::string)
    CMD(ltrim, std::string, int, int)
    CMD(rpop, std::string)
    CMD(rpoplpush, std::string, std::string)
    CMD(rpush, std::vector<std::string>)
    CMD(rpushx, std::string, std::string)

    // Sorted sets
    CMD(zadd, std::vector<std::string>)
    CMD(zcard, std::string)
    CMD(zcount, std::string, float, float)
    CMD(zincrby, std::string, float, std::string)
    CMD(zinterstore, std::vector<std::string>)
    CMD(zrange, std::string, int, int)
    CMD(zrange, std::string, int, int, std::string)
    //CMD(zrangebyscore, std::string, std::string, std::string)
    //CMD(zrangebyscore, std::string, std::string, std::string, std::string)
    //CMD(zrangebyscore, std::string, std::string, std::string, std::string, unsigned, unsigned)
    //CMD(zrangebyscore, std::string, std::string, std::string, std::string, std::string, unsigned, unsigned)
    CMD(zrank, std::string, std::string)
    CMD(zrem, std::vector<std::string>)
    CMD(zremrangebyrank, std::string, int, int)
    CMD(zremrangebyscore, std::string, float, float)
    CMD(zrevrange, std::string, int, int)
    CMD(zrevrange, std::string, int, int, std::string)
    //CMD(zrevrangebyscore, std::string, std::string, std::string)
    //CMD(zrevrangebyscore, std::string, std::string, std::string, std::string)
    //CMD(zrevrangebyscore, std::string, std::string, std::string, std::string, unsigned, unsigned)
    //CMD(zrevrangebyscore, std::string, std::string, std::string, std::string, std::string, unsigned, unsigned)
    CMD(zrevrank, std::string, std::string)
    CMD(zscore, std::string, std::string)
    CMD(zunionstore, std::vector<std::string>)

    // Pub-sub, commands visible outside of a subscribed session

    commands["publish"] = new pubsub_publish_cmd(this);
    commands["subscribe"] = new pubsub_subscribe_cmd(this, false);
    commands["psubscribe"] = new pubsub_subscribe_cmd(this, true);

    //TODO should just return 0
    //commansd["unsubscribe"] = new ;
    //commands["punsubscribe"] = new ;
}

void RedisParser::parseCommand(tcp_conn_t *conn) {
    const char *error_msg = NULL;
    try {
        LineParser p(conn);

        // All commands begin with a first line of '*{args}\r\n'
        std::string line = p.readLine();
        int argNum = parseArgNum(line);

        // In this case we're done, no arguments to read
        if (argNum <= 0) return;

        // Now we parse argNum args

        // The first arg is always the command_t
        std::string command = parseArg(p);
        boost::algorithm::to_lower(command);

        // Note: Redis parses all expected lines before trying to match a command

        std::vector<std::string> args;
        for (int i = 0; i < argNum - 1; ++i) {
            args.push_back(parseArg(p));
        }

        // Match the command
        command_t *result = commands.at(command);

        // Run the command
        output_response(result->execute(this, args));

    } catch (std::out_of_range) {
        error_msg = "Command not recognized";
    } catch (ParseError &e) {
        error_msg = "Parse error";
    }

    // We can't call output_error inside the catch block so we have this little workaround
    if (error_msg) {
        output_error(error_msg);
    }
}

int RedisParser::parseNumericLine(char prefix, std::string &line) {
    if (!(line[0] == prefix)) {
        // line must begin with prefix
        throw ParseError();
    }
    line.erase(0, 1);

    int numNum;
    try {
        numNum = boost::lexical_cast<int>(line);
    } catch (boost::bad_lexical_cast) {
        // Value on this line is not a unsigned integer value
        throw ParseError();
    }

    return numNum;
}

int RedisParser::parseArgNum(std::string &line) {
    return parseNumericLine('*', line);
}

int RedisParser::parseArgLength(std::string &line) {
    return parseNumericLine('$', line);
}

std::string RedisParser::parseArg(LineParser &p) {
    std::string line = p.readLine();

    // Each arg begins with the line '${argLength}\r\n'
    int argLength = parseArgLength(line);

    // Error, no valid length
    if (argLength < 0) throw ParseError();

    return p.readLineOf(argLength);
}

void RedisParser::first_subscribe(std::vector<std::string> &channels, bool patterned) {
    subscribe(channels, patterned);

    // After subscription we only accept pub/sub commands until unsubscribe
    parse_pubsub();
}

void RedisParser::subscribe(std::vector<std::string> &channels, bool patterned) {
    for (std::vector<std::string>::iterator iter = channels.begin(); iter != channels.end(); iter++) {
        // Add our subscription
        if (patterned) {
            subscribed_channels += runtime->subscribe_pattern(*iter, connection_id, this);
        } else {
            subscribed_channels += runtime->subscribe(*iter, connection_id, this);
        }

        // Output the successful subscription message
        std::vector<std::string> subscription_message;
        subscription_message.push_back(std::string("subscribe"));
        subscription_message.push_back(*iter);
        subscription_message.push_back(boost::lexical_cast<std::string>(subscribed_channels));
        output_response(redis_protocol_t::redis_return_type(subscription_message));
    }
}

void RedisParser::unsubscribe(std::vector<std::string> &channels, bool patterned) {
    for (std::vector<std::string>::iterator iter = channels.begin(); iter != channels.end(); iter++) {
        // Add our subscription
        if (patterned) {
            subscribed_channels -= runtime->unsubscribe_pattern(*iter, connection_id);
        } else {
            subscribed_channels -= runtime->unsubscribe(*iter, connection_id);
        }

        // Output the sucessful unsubscription message
        std::vector<std::string> unsubscription_message;
        unsubscription_message.push_back(std::string("unsubscribe"));
        unsubscription_message.push_back(*iter);
        unsubscription_message.push_back(boost::lexical_cast<std::string>(subscribed_channels));
        output_response(redis_protocol_t::redis_return_type(unsubscription_message));
    }
}

redis_protocol_t::redis_return_type RedisParser::publish(std::string &channel, std::string &message) {
    // This behaves like a normal command. No special states associated here
    int clients_recieved = runtime->publish(channel, message);
    return redis_protocol_t::redis_return_type(clients_recieved);
};

void RedisParser::parse_pubsub() {
    // loop on parse. Parse just pub/sub commands

    try {
        LineParser p(conn);

        while (subscribed_channels > 0) {
            std::string line = p.readLine();
            int argNum = parseArgNum(line);

            if (argNum <= 0) return;

            std::string command = parseArg(p);
            boost::algorithm::to_lower(command);

            std::vector<std::string> chans;
            for (int i = 0; i < argNum - 1; ++i) {
                chans.push_back(parseArg(p));
            }

            if (command == "subscribe") {
                subscribe(chans, false);
            } else if (command == "psubscribe") {
                subscribe(chans, true);
            } else if (command == "unsubscribe") {
                unsubscribe(chans, false);
            } else if (command == "punsubscribe") {
                unsubscribe(chans, true);
            } else {
                output_response(redis_protocol_t::error_result(
                    "ERR only (P)SUBSCRIBE / (P)UNSUBSCRIBE / QUIT allowed in this contex"
                ));
            }
        }
    } catch (ParseError &e) {
        //TODO what to do here?
    }
}

void start_serving(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *intface, pubsub_runtime_t *runtime) {
    RedisParser redis(conn, intface, runtime);
    try {
        while (true) {
            redis.parseCommand(conn);
        }
    } catch (tcp_conn_t::read_closed_exc_t) {}

}

// TODO permanent solution
pubsub_runtime_t pubsub_runtime;

//The entry point for the parser. The only requirement for a parser is that it implement this function.
void serve_redis(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *redis_intf) {
    start_serving(conn, redis_intf, &pubsub_runtime);
}
