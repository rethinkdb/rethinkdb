#ifndef RIAK_PROTO_SERVER_HPP_
#define RIAK_PROTO_SERVER_HPP_

#include "arch/arch.hpp"
#include "riak/proto/riakclient.pb.h"


namespace riak {

class riak_interface_t;

namespace dummy_msgs {
    //Dummy "messages" that riak sends that are 0 length, unfortunately they
    //don't actually create types for these which puts the kibosh on my
    //overloaded function scheme, so here are some dummy messages so I have
    //some types to work with

    struct dummy_msg_t {
        bool ParseFromArray(const char *, size_t size) { return size == 0; }
    };
    class RpbPingReq : public dummy_msg_t {};
    class RpbPingResp : public dummy_msg_t {};
    class RpbGetClientIdReq : public dummy_msg_t {};
    class RpbSetClientIdResp : public dummy_msg_t {};
    class RpbGetServerInfoReq : public dummy_msg_t {};
    class RpbListBucketsReq : public dummy_msg_t {};
    class RpbDelResp : public dummy_msg_t {};
    class RpbSetBucketResp : public dummy_msg_t {};

} // namespace dummy_msgs

class proto_server_t {
private:
    boost::scoped_ptr<tcp_listener_t> tcp_listener;
    void handle_conn(boost::scoped_ptr<nascent_tcp_conn_t> &conn);
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

    message_code code(::RpbErrorResp const &) { return RpbErrorResp; }
    message_code code(dummy_msgs::RpbPingReq const &) {return  RpbPingReq; }
    message_code code(dummy_msgs::RpbPingResp const &) { return RpbPingResp; }
    message_code code(dummy_msgs::RpbGetClientIdReq const &)  { return RpbGetClientIdReq; } 
    message_code code(::RpbGetClientIdResp const &) { return RpbGetClientIdResp; } 
    message_code code(::RpbSetClientIdReq const &) { return RpbSetClientIdReq; } 
    message_code code(dummy_msgs::RpbSetClientIdResp const &) { return RpbSetClientIdResp; } 
    message_code code(dummy_msgs::RpbGetServerInfoReq const &) { return RpbGetServerInfoReq; } 
    message_code code(::RpbGetServerInfoResp const &) { return RpbGetServerInfoResp; } 
    message_code code(::RpbGetReq const &) { return RpbGetReq; } 
    message_code code(::RpbGetResp const &) { return RpbGetResp; } 
    message_code code(::RpbPutReq const &) { return RpbPutReq; } 
    message_code code(::RpbPutResp const &) { return RpbPutResp; } 
    message_code code(::RpbDelReq const &) { return RpbDelReq; } 
    message_code code(dummy_msgs::RpbDelResp const &) { return RpbDelResp; } 
    message_code code(dummy_msgs::RpbListBucketsReq const &) { return RpbListBucketsReq; } 
    message_code code(::RpbListBucketsResp const &) { return RpbListBucketsResp; } 
    message_code code(::RpbListKeysReq const &) { return RpbListKeysReq; } 
    message_code code(::RpbListKeysResp const &) { return RpbListKeysResp; } 
    message_code code(::RpbGetBucketReq const &) { return RpbGetBucketReq; } 
    message_code code(::RpbGetBucketResp const &) { return RpbGetBucketResp; } 
    message_code code(::RpbSetBucketReq const &) { return RpbSetBucketReq; } 
    message_code code(dummy_msgs::RpbSetBucketResp const &) { return RpbSetBucketResp; } 
    message_code code(::RpbMapRedReq const &) { return RpbMapRedReq; } 
    message_code code(::RpbMapRedResp const &) { return RpbMapRedResp; } 

    template <class T>
    void write_to_conn(T const &t, boost::scoped_ptr<tcp_conn_t> &conn) {
        message_size_t size = t.ByteSize();
        message_code_t mc = code(t); 

        conn->write(&size, sizeof(message_size_t));
        conn->write(&mc, sizeof(message_code_t));
        std::string str;
        rassert(t.SerializeToString(&str));
        rassert(str.size() == (size_t) t.ByteSize());

        conn->write(str.data(), str.size());
    }

private:
    //parse a msg of a type T and call handle_buffer
    template <class T>
    void handle(const_charslice &, boost::scoped_ptr<tcp_conn_t> &);

private:
    void handle_msg(dummy_msgs::RpbPingReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(dummy_msgs::RpbGetClientIdReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(::RpbSetClientIdReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(dummy_msgs::RpbGetServerInfoReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(::RpbGetReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(::RpbPutReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(::RpbDelReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(dummy_msgs::RpbListBucketsReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(::RpbListKeysReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(::RpbGetBucketReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(::RpbSetBucketReq &, boost::scoped_ptr<tcp_conn_t> &);
    void handle_msg(::RpbMapRedReq &, boost::scoped_ptr<tcp_conn_t> &);
};

} //namespace riak

#endif
