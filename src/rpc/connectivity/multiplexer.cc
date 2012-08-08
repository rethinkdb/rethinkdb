#include "rpc/connectivity/multiplexer.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "rpc/connectivity/connectivity.hpp"

message_multiplexer_t::run_t::run_t(message_multiplexer_t *p) : parent(p) {
    rassert(parent->run == NULL);
    parent->run = this;
    for (int i = 0; i < max_tag; ++i) {
        if (parent->clients[i]) {
            rassert(parent->clients[i]->run);
        }
    }
}

message_multiplexer_t::run_t::~run_t() {
    rassert(parent->run == this);
    parent->run = NULL;
}

void message_multiplexer_t::run_t::on_message(peer_id_t source, read_stream_t *stream) {
    tag_t tag;
    archive_result_t res = deserialize(stream, &tag);
    if (res) { throw fake_archive_exc_t(); }
    client_t *client = parent->clients[tag];
    guarantee(client != NULL, "Got a message for an unfamiliar tag. Apparently "
        "we aren't compatible with the cluster on the other end.");
    client->run->message_handler->on_message(source, stream);
}

message_multiplexer_t::client_t::run_t::run_t(client_t *c, message_handler_t *m) :
    parent(c), message_handler(m)
{
    rassert(parent->parent->run == NULL);
    rassert(parent->run == NULL);
    parent->run = this;
}

message_multiplexer_t::client_t::run_t::~run_t() {
    rassert(parent->parent->run == NULL);
    rassert(parent->run == this);
    parent->run = NULL;
}

message_multiplexer_t::client_t::client_t(message_multiplexer_t *p, tag_t t) :
    parent(p), tag(t), run(NULL)
{
    rassert(parent->run == NULL);
    rassert(parent->clients[tag] == NULL);
    parent->clients[tag] = this;
}

message_multiplexer_t::client_t::~client_t() {
    rassert(parent->run == NULL);
    rassert(parent->clients[tag] == this);
    parent->clients[tag] = NULL;
}

connectivity_service_t *message_multiplexer_t::client_t::get_connectivity_service() {
    return parent->message_service->get_connectivity_service();
}

void write_tagged_message(write_stream_t *os, message_multiplexer_t::tag_t tag, const boost::function<void(write_stream_t *)> &subwriter) {
    write_message_t msg;
    msg << tag;
    int res = send_write_message(os, &msg);
    if (res) { throw fake_archive_exc_t(); }
    subwriter(os);
}

void message_multiplexer_t::client_t::send_message(peer_id_t dest, const boost::function<void(write_stream_t *)> &subwriter) {
    parent->message_service->send_message(dest,
        boost::bind(&write_tagged_message, _1, tag, boost::cref(subwriter))
        );
}

message_multiplexer_t::message_multiplexer_t(message_service_t *super_ms) :
    message_service(super_ms), run(NULL)
{
    for (int i = 0; i < max_tag; ++i) {
        clients[i] = NULL;
    }
}

message_multiplexer_t::~message_multiplexer_t() {
    rassert(run == NULL);
    for (int i = 0; i < max_tag; ++i) {
        rassert(clients[i] == NULL);
    }
}

