#include "rpc/connectivity/multiplexer.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "rpc/connectivity/connectivity.hpp"

message_multiplexer_t::run_t::run_t(message_multiplexer_t *p) : parent(p) {
    guarantee(parent->run == NULL);
    parent->run = this;
#ifndef NDEBUG
    for (int i = 0; i < max_tag; i++) {
        if (parent->clients[i]) {
            rassert(parent->clients[i]->run);
        }
    }
#endif  // NDEBUG
}

message_multiplexer_t::run_t::~run_t() {
    guarantee(parent->run == this);
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
    guarantee(parent->parent->run == NULL);
    guarantee(parent->run == NULL);
    parent->run = this;
}

message_multiplexer_t::client_t::run_t::~run_t() {
    guarantee(parent->parent->run == NULL);
    guarantee(parent->run == this);
    parent->run = NULL;
}

message_multiplexer_t::client_t::client_t(message_multiplexer_t *p, tag_t t) :
    parent(p), tag(t), run(NULL)
{
    guarantee(parent->run == NULL);
    guarantee(parent->clients[tag] == NULL);
    parent->clients[tag] = this;
}

message_multiplexer_t::client_t::~client_t() {
    guarantee(parent->run == NULL);
    guarantee(parent->clients[tag] == this);
    parent->clients[tag] = NULL;
}

connectivity_service_t *message_multiplexer_t::client_t::get_connectivity_service() {
    return parent->message_service->get_connectivity_service();
}

class tagged_message_writer_t : public send_message_write_callback_t {
public:
    tagged_message_writer_t(message_multiplexer_t::tag_t _tag, send_message_write_callback_t *_subwriter) :
        tag(_tag), subwriter(_subwriter) { }
    virtual ~tagged_message_writer_t() { }

    void write(write_stream_t *os) {
        write_message_t msg;
        msg << tag;
        int res = send_write_message(os, &msg);
        if (res) { throw fake_archive_exc_t(); }
        subwriter->write(os);
    }

private:
    message_multiplexer_t::tag_t tag;
    send_message_write_callback_t *subwriter;
};

void message_multiplexer_t::client_t::send_message(peer_id_t dest, send_message_write_callback_t *callback) {
    tagged_message_writer_t writer(tag, callback);
    parent->message_service->send_message(dest, &writer);
}

message_multiplexer_t::message_multiplexer_t(message_service_t *super_ms) :
    message_service(super_ms), run(NULL)
{
    for (int i = 0; i < max_tag; i++) {
        clients[i] = NULL;
    }
}

message_multiplexer_t::~message_multiplexer_t() {
    guarantee(run == NULL);
    for (int i = 0; i < max_tag; i++) {
        guarantee(clients[i] == NULL);
    }
}

