#ifndef __RIAK_PROTO_SERVER_HPP__
#define __RIAK_PROTO_SERVER_HPP__

#include "arch/arch.hpp"
#include "riak/proto/riakclient.pb.h"

class riak_interface_t;

namespace riak {

namespace dummy_msgs {
    //Dummy "messages" that riak sends that are 0 length, unfortunately they
    //don't actually create types for these which puts the kibosh on my
    //overloaded function scheme, so here are some dummy messages so I have
    //some types to work with

    struct dummy_msg_t {
        bool ParseFromArray(const char *, size_t size) { return size == 0; }
    };
    class RpbPingReq : public dummy_msg_t {};
    class RpbGetClientIdReq : public dummy_msg_t {};
    class RpbGetServerInfoReq : public dummy_msg_t {};
    class RpbListBucketsReq : public dummy_msg_t {};

} // namespace dummy_msgs

class proto_server_t {
private:
    boost::scoped_ptr<tcp_listener_t> tcp_listener;
    void handle_conn(boost::scoped_ptr<tcp_conn_t> &conn);
    riak_interface_t *riak_interface;

public:
    proto_server_t(int port, riak_interface_t *);
    ~proto_server_t();

private:

    //the message code that's sent over the wire
    typedef uint16_t message_code_t;

    //the message size that's sent over the wire
    typedef uint32_t message_size_t;

    enum  message_code {
        RpbErrorResp = 0,
        RpbPingReq = 1,
        RpbPingResp = 2,
        RpbGetClientIdReq = 3,
        RpbGetClientIdResp = 4,
        RpbSetClientIdReq = 5,
        RpbSetClientIdResp = 6,
        RpbGetServerInfoReq = 7,
        RpbGetServerInfoResp = 8,
        RpbGetReq = 9,
        RpbGetResp = 10,
        RpbPutReq = 11,
        RpbPutResp = 12,
        RpbDelReq = 13,
        RpbDelResp = 14,
        RpbListBucketsReq = 15,
        RpbListBucketsResp = 16,
        RpbListKeysReq = 17,
        RpbListKeysResp = 18,
        RpbGetBucketReq = 19,
        RpbGetBucketResp = 20,
        RpbSetBucketReq = 21,
        RpbSetBucketResp = 22,
        RpbMapRedReq = 23,
        RpbMapRedResp = 24
    };

private:
    //parse a msg of a type T and call handle_buffer
    template <class T>
    void handle(const_charslice &);

private:
    void handle_msg(dummy_msgs::RpbPingReq &);
    void handle_msg(dummy_msgs::RpbGetClientIdReq &);
    void handle_msg(::RpbSetClientIdReq &);
    void handle_msg(dummy_msgs::RpbGetServerInfoReq &);
    void handle_msg(::RpbGetReq &);
    void handle_msg(::RpbPutReq &);
    void handle_msg(::RpbDelReq &);
    void handle_msg(dummy_msgs::RpbListBucketsReq &);
    void handle_msg(::RpbListKeysReq &);
    void handle_msg(::RpbGetBucketReq &);
    void handle_msg(::RpbSetBucketReq &);
    void handle_msg(::RpbMapRedReq &);
};

} //namespace riak

#endif
