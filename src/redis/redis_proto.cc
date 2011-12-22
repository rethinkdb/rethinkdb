#include "errors.hpp"
#include <boost/variant.hpp>
#include <boost/unordered_map.hpp>
#include <boost/algorithm/string.hpp>

//Spirit QI
//Note this result could have been achieved with <boost/spirit/include/qi.hpp> but I expanded
//it out in the hopes that this would reduce compile time, it did by about half a second
//#include <boost/spirit/home/qi/parse.hpp>

#include "redis/pubsub.hpp"
#include "redis/redis.hpp"
#include "redis/redis_proto.hpp"
#include "redis/redis_ext.hpp"
//#include "redis/redis_grammar.hpp"

class RedisParseError {
public:
    virtual ~RedisParseError() {}
    virtual const char *what() const throw() {
        return "Redis parse failure";
    }
} parseException;

class RedisTypeError : public RedisParseError {
public:
    virtual const char *what() const throw() {
        return "Wrong number of arguments applied to Redis command";
    }
} typeException;

class phantom {};

class command_t {
public:
    virtual ~command_t() {}

    virtual redis_protocol_t::redis_return_type execute(redis_ext *extApi, std::vector<std::string> &args) {
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

    virtual redis_protocol_t::redis_return_type operator()(UNUSED redis_ext *extApi) {
        throw typeException;
    }

    virtual redis_protocol_t::redis_return_type operator()(UNUSED redis_ext *extApi, UNUSED std::string& arg1_string) {
        throw typeException;
    }

    virtual redis_protocol_t::redis_return_type operator()(UNUSED redis_ext *extApi, UNUSED std::string& arg1_string, UNUSED std::string& arg2_string) {
        throw typeException;
    }

    virtual redis_protocol_t::redis_return_type operator()(UNUSED redis_ext *extApi, UNUSED std::string& arg1_string, UNUSED std::string& arg2_string, UNUSED std::string& arg3_string) {
        throw typeException;
    }

    virtual redis_protocol_t::redis_return_type operator()(UNUSED redis_ext *extApi, UNUSED std::string& arg1_string, UNUSED std::string& arg2_string, UNUSED std::string& arg3_string, UNUSED std::string &arg4_string) {
        throw typeException;
    }
};

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
        } catch(boost::bad_lexical_cast) {
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
        } catch(boost::bad_lexical_cast) {
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
        } catch(boost::bad_lexical_cast) {
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
        } catch(boost::bad_lexical_cast) {
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

#define CMD(NAME, ...) commands[#NAME] = new cmd<__VA_ARGS__>(&redis_ext::NAME);

class RedisParser: public redis_ext, private redis_output_writer {
private:
    boost::unordered_map<const std::string, command_t*> commands;
    pubsub_runtime_t *runtime;

public:
    RedisParser(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *intf, pubsub_runtime_t *runtime_) :
         redis_ext(intf),
         redis_output_writer(conn),
         runtime(runtime_)
    {
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

    }

    void parseCommand(tcp_conn_t *conn) {
        const char *error_msg = NULL;
        try {
            LineParser p(conn);

            // All commands begin with a first line of '*{args}\r\n'
            std::string line = p.readLine();
            int argNum = parseArgNum(line);

            // In this case we're done, no arguments to read
            if(argNum <= 0) return;

            // Now we parse argNum args

            // The first arg is always the command_t
            std::string command = parseArg(p);
            boost::algorithm::to_lower(command);

            // Note: Redis parses all expected lines before trying to match a command

            std::vector<std::string> args;
            for(int i=0; i<argNum-1; ++i) {
                args.push_back(parseArg(p));
            }

            // Match the command
            command_t *result = commands.at(command);

            // Run the command
            output_response(result->execute(this, args));

        } catch(std::out_of_range) {
            error_msg = "Command not recognized";
        } catch(RedisParseError &e) {
            error_msg = "Parse error";
        }

        // We can't call output_error inside the catch block so we have this little workaround
        if(error_msg) {
            output_error(error_msg);
        }
    }

private:
    // Parses CRLF terminated lines from a TCP connection
    class LineParser {
    private:
        tcp_conn_t *conn;

        const char *start_position;
        unsigned bytes_read;
        const char *end_position;

    public:
        explicit LineParser(tcp_conn_t *conn_) : conn(conn_) {
            peek();
            bytes_read = 0;
        }

        // Returns a charslice to the next CRLF line in the TCP conn's buffer
        // blocks until a full line is available
        std::string readLine() {
            while(!readCRLF()) {
                bytes_read++;
            }

            // Construct a string of everything upto the CRLF
            std::string line(start_position, bytes_read - 2);
            pop();
            return line;
        }

        // Both ensures that next line read is exactly bytes long and is able
        // to operate more efficiently than readLine.
        std::string readLineOf(size_t bytes) {
            // Since we know exactly how much we want to read we'll buffer ourselves
            // +2 is for expected terminating CRLF
            char *buff = static_cast<char *>(alloca(bytes + 2));
            conn->read(buff, bytes + 2);

            // Now we expect the line to be demarcated by a CRLF
            if((buff[bytes] == '\r') && (buff[bytes+1] == '\n')) {
                return std::string(buff, buff + bytes);
            } else {
                // Error: line incorrectly terminated.
                /* Note: Redis behavior seems to be to assume that the line is
                   always correctly terminated. It will just take the rest of
                   the input as the start of the next line and then probably
                   end up in an error state. We can probably be more fastidious.
                */
                throw parseException;
            }

            // No need to pop or anything. Did not use tcp_conn's buffer or bytes_read.
        }

        

    private:
        void peek() {
            const_charslice slice = conn->peek();
            start_position = slice.beg;
            end_position = slice.end;
        }

        void pop() {
            conn->pop(bytes_read);
            peek();
            bytes_read = 0;
        }

        char current() {
            while(bytes_read >= (end_position - start_position)) {
                conn->read_more_buffered();
                peek();
            }
            return start_position[bytes_read];
        }

        // Attempts to read a crlf at the current position
        // possibily advanes the current position in its attempt to do so
        bool readCRLF() {
            if(current() == '\r') {
                bytes_read++;

                if(current() == '\n') {
                    bytes_read++;
                    return true;
                }
            }
            return false;
        }
    };

    int parseNumericLine(char prefix, std::string &line) {
        if(!(line[0] == prefix)) {
            // line must begin with prefix
            throw parseException;
        }
        line.erase(0, 1);

        int numNum;
        try {
            numNum = boost::lexical_cast<int>(line);
        } catch(boost::bad_lexical_cast) {
            // Value on this line is not a unsigned integer value
            throw parseException;
        }

        return numNum;
    }

    int parseArgNum(std::string &line) {
        return parseNumericLine('*', line);
    }

    int parseArgLength(std::string &line) {
        return parseNumericLine('$', line);
    }

    std::string parseArg(LineParser &p) {
        std::string line = p.readLine();

        // Each arg begins with the line '${argLength}\r\n'
        int argLength = parseArgLength(line);

        // Error, no valid length
        if(argLength < 0) throw parseException;

        return p.readLineOf(argLength);
    }
};

void start_serving(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *intface, pubsub_runtime_t *runtime) {
    RedisParser redis(conn, intface, runtime);
    try {
        while(true) {
            redis.parseCommand(conn);
        }
    } catch(tcp_conn_t::read_closed_exc_t) {}

}

// TODO permanent solution
pubsub_runtime_t pubsub_runtime;

//The entry point for the parser. The only requirement for a parser is that it implement this function.
void serve_redis(tcp_conn_t *conn, namespace_interface_t<redis_protocol_t> *redis_intf) {
    start_serving(conn, redis_intf, &pubsub_runtime);
}

//Output functions
//These take the results of a redis_interface_t method and send them out on the wire.
//Handling both input and output here means that this class is solely responsible for
//translating the redis protocol.

struct output_visitor : boost::static_visitor<void> {
    explicit output_visitor(tcp_conn_t *conn) : out_conn(conn) {;}
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
        out_conn->write(res.c_str(), res.size());
        out_conn->write("\r\n", 2);

    }

    void operator()(std::string &res) const {
        bulk_result(res);
    }

    void operator()(redis_protocol_t::nil_result) const {
        out_conn->write("$-1\r\n", 5);
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

void redis_output_writer::output_response(redis_protocol_t::redis_return_type response) {
    boost::apply_visitor(output_visitor(out_conn), response);
}

void redis_output_writer::output_error(const char *error) {
    output_response(redis_protocol_t::error_result(error));
}
