#ifndef PROTOCOL_REDIS_PROTO_HPP_
#define PROTOCOL_REDIS_PROTO_HPP_

#include <boost/unordered_map.hpp>

#include "arch/types.hpp"
#include "redis/redis.hpp"
#include "redis/pubsub.hpp"
#include "redis/redis_ext.hpp"
#include "redis/redis_output.hpp"
#include "parsing/util.hpp"

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

#endif /* PROTOCOL_REDIS_PROTO_HPP_*/
