#ifndef __PROTOCOL_REDIS_PROTO_HPP__
#define __PROTOCOL_REDIS_PROTO_HPP__

#include <boost/unordered_map.hpp>

#include "arch/types.hpp"
#include "redis/redis.hpp"
#include "redis/pubsub.hpp"
#include "redis/redis_ext.hpp"
#include "redis/redis_output.hpp"

template <class protocol_type>
class namespace_interface_t;

void serve_redis(tcp_conn_t *, namespace_interface_t<redis_protocol_t> *);

class command_t {
public:
    virtual ~command_t() {}
    virtual redis_protocol_t::redis_return_type execute(redis_ext *extApi, std::vector<std::string> &args); 
    virtual redis_protocol_t::redis_return_type operator()(redis_ext *extApi); 
    virtual redis_protocol_t::redis_return_type operator()(redis_ext *extApi, std::string& arg1_string); 
    virtual redis_protocol_t::redis_return_type operator()(redis_ext *extApi, std::string& arg1_string, std::string& arg2_string); 
    virtual redis_protocol_t::redis_return_type operator()(redis_ext *extApi, std::string& arg1_string, std::string& arg2_string, std::string& arg3_string); 
    virtual redis_protocol_t::redis_return_type operator()(redis_ext *extApi, std::string& arg1_string, std::string& arg2_string, std::string& arg3_string, std::string &arg4_string);
};

class RedisParser: public redis_ext, private redis_output_writer {
private:
    boost::unordered_map<const std::string, command_t*> commands;
    pubsub_runtime_t *runtime;
    tcp_conn_t *conn;

public:
    RedisParser(tcp_conn_t *conn_, namespace_interface_t<redis_protocol_t> *intf, pubsub_runtime_t *runtime_);
    void parseCommand(tcp_conn_t *conn);

private:
    // Parses CRLF terminated lines from a TCP connection
    class LineParser {
    private:
        tcp_conn_t *conn;

        const char *start_position;
        unsigned bytes_read;
        const char *end_position;

    public:
        explicit LineParser(tcp_conn_t *conn_);

        // Returns a charslice to the next CRLF line in the TCP conn's buffer
        // blocks until a full line is available
        std::string readLine(); 

        // Both ensures that next line read is exactly bytes long and is able
        // to operate more efficiently than readLine.
        std::string readLineOf(size_t bytes);

    private:
        void peek();
        void pop();
        char current();

        // Attempts to read a crlf at the current position
        // possibily advanes the current position in its attempt to do so
        bool readCRLF();
    };

    int parseNumericLine(char prefix, std::string &line);
    int parseArgNum(std::string &line);
    int parseArgLength(std::string &line);
    std::string parseArg(LineParser &p);

    // Pub/sub support

    // Randomly generated (and so hopefully unique) id for this connection. Used to identify this connection
    // when subscribing and unscribing from channels
    uint64_t connection_id;
    uint64_t subscribed_channels;

public:
    void first_subscribe(std::vector<std::string> &channels, bool patterned);
    redis_protocol_t::redis_return_type publish(std::string &channel, std::string &message);

private:
    void subscribe(std::vector<std::string> &channels, bool patterned);
    void unsubscribe(std::vector<std::string> &channels, bool patterned);

private:
    void parse_pubsub();
};

#endif /* __PROTOCOL_REDIS_PROTO_HPP__*/
