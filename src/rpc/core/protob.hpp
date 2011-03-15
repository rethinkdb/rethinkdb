#ifndef _RPC_CORE_PROTOB_HPP_
#define _RPC_CORE_PROTOB_HPP_

#include "arch/arch.hpp"
#include <google/protobuf/message.h>
#include "rpc/core/header.pb.h"
#include "utils.hpp"
#include "utils2.hpp"
#include <string>
#include <vector>
#include <utility>
#include "logger.hpp"

using namespace google;
using namespace protobuf;

void hd_packet(header::hdr hdr);

/* returns a pointer to the message that parsed  (NULL if none do) */
bool peek_protob(tcp_conn_t *conn, Message *msg);

/* pop a packet of the socket (doesn't care about the packet type) optionally a
 * hdr can be given in which case the data will be parsed in to it, you should
 * always know what type of packet you're trying to parse off the socket, this
 * should only be used for debugging to print the type of a packet that you
 * weren't expecting */
void pop_protob(tcp_conn_t *conn, header::hdr *hdr = NULL);

/* read a packet of this type of the socket (returns false if the packet found is not of this type) */
bool read_protob(tcp_conn_t *conn, Message *msg);

void write_protob(tcp_conn_t *conn, Message *msg); 

#endif
