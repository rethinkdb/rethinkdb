#ifndef __RPC_CONNECTIVITY_PROTOCOL_HPP__
#define __RPC_CONNECTIVITY_PROTOCOL_HPP__

namespace connectivity_protocol {

/* The protocol is as follows: When a connection is established, each peer
sends the other peer its own ID and a list of all the peers it knows about.
This is described by the `intro_t` struct. After that, everything on the pipe
is a `message_t`. The `contents` of the `message_t` are interpreted by the
next layer up the stack; to the connectivity logic, they are an opaque block.
*/

struct address_t {
    std::string host;
    int port;
};

struct intro_t {
    boost::uuid my_id;
    std::map<boost::uuid, address_t> my_peers;
};

struct message_t {
    std::string contents;
};

}

#endif /* __RPC_CONNECTIVITY_PROTOCOL_HPP__ */
