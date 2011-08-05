#include "riak/proto_server.hpp"
#include "riak/proto/riakclient.pb.h"
#include <google/protobuf/message.h>

namespace riak {

void proto_server_t::handle_conn(boost::scoped_ptr<tcp_conn_t> &conn) {
    for (;;) {
        message_size_t size;
        message_code_t mc;

        conn->read(&size, sizeof(size));
        conn->read(&mc, sizeof(mc));

        const_charslice slc = conn->peek(size);

        switch (mc) {
        case RpbPingReq:
            handle<dummy_msgs::RpbPingReq>(slc);
            break;
        case RpbGetClientIdReq:
            handle<dummy_msgs::RpbGetClientIdReq>(slc);
            break;
        case RpbSetClientIdReq:
            handle< ::RpbSetClientIdReq>(slc);
            break;
        case RpbGetServerInfoReq:
            handle<dummy_msgs::RpbGetServerInfoReq>(slc);
            break;
        case RpbGetReq:
            handle< ::RpbGetReq>(slc);
            break;
        case RpbPutReq:
            handle< ::RpbPutReq>(slc);
            break;
        case RpbDelReq:
            handle< ::RpbDelReq>(slc);
            break;
        case RpbListBucketsReq:
            handle<dummy_msgs::RpbListBucketsReq>(slc);
            break;
        case RpbListKeysReq:
            handle< ::RpbListKeysReq>(slc);
            break;
        case RpbGetBucketReq:
            handle< ::RpbGetBucketReq>(slc);
            break;
        case RpbSetBucketReq:
            handle< ::RpbSetBucketReq>(slc);
            break;
        case RpbMapRedReq:
            handle< ::RpbMapRedReq>(slc);
            break;
        default:
            unreachable();
        }

        conn->pop(size);
    }
}

proto_server_t::proto_server_t(int port, riak_interface_t *riak_interface)
    : riak_interface(riak_interface)
{ 
    tcp_listener.reset(new tcp_listener_t(port, boost::bind(&proto_server_t::handle_conn, this, _1)));
}

proto_server_t::~proto_server_t() { }

template <class T>
void proto_server_t::handle(const_charslice &slc) {
    T msg;
    msg.ParseFromArray(slc.beg, slc.end - slc.beg);
    //handle(msg);
}

} //namespace riak
