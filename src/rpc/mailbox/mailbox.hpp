// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_MAILBOX_MAILBOX_HPP_
#define RPC_MAILBOX_MAILBOX_HPP_

#include <map>
#include <string>
#include <vector>

#include "backtrace.hpp"
#include "concurrency/new_semaphore.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/vector_stream.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/mailbox/raw_mailbox.hpp"
#include "rpc/semilattice/joins/macros.hpp"

class mailbox_manager_t;

/* `send_write()` sends a message to a mailbox. `send_write()` can block and must be called
in a coroutine. If the mailbox does not exist or the peer is disconnected, `send_write()`
will silently fail. Mailbox messages are not necessarily delivered in order. */

void send_write(mailbox_manager_t *src,
                raw_mailbox_t::address_t dest,
                mailbox_write_callback_t *callback);

/* `mailbox_manager_t` is a `cluster_message_handler_t` that takes care
of actually routing messages to mailboxes. */

class mailbox_manager_t : public cluster_message_handler_t {
public:
    mailbox_manager_t(connectivity_cluster_t *connectivity_cluster,
                      connectivity_cluster_t::message_tag_t message_tag);

private:
    friend class raw_mailbox_t;
    friend void send_write(mailbox_manager_t *, raw_mailbox_t::address_t, mailbox_write_callback_t *callback);

    struct mailbox_table_t {
        mailbox_table_t();
        ~mailbox_table_t();
        raw_mailbox_t::id_t next_mailbox_id;
        // TODO: use a buffered structure to reduce dynamic allocation
        std::map<raw_mailbox_t::id_t, raw_mailbox_t *> mailboxes;
        raw_mailbox_t *find_mailbox(raw_mailbox_t::id_t);
    };
    one_per_thread_t<mailbox_table_t> mailbox_tables;

    /* We must acquire one of these semaphores whenever we want to send a message over a
    mailbox. This prevents mailbox messages from starving directory and semilattice
    messages. */
    one_per_thread_t<new_semaphore_t> semaphores;

    raw_mailbox_t::id_t generate_mailbox_id();

    raw_mailbox_t::id_t register_mailbox(raw_mailbox_t *mb);
    void unregister_mailbox(raw_mailbox_t::id_t id);

    static void write_mailbox_message(write_stream_t *stream,
                                      threadnum_t dest_thread,
                                      raw_mailbox_t::id_t dest_mailbox_id,
                                      mailbox_write_callback_t *callback);

    void on_message(connectivity_cluster_t::connection_t *connection,
                    auto_drainer_t::lock_t connection_keeepalive,
                    read_stream_t *stream);
    void on_local_message(connectivity_cluster_t::connection_t *connection,
                          auto_drainer_t::lock_t connection_keepalive,
                          std::vector<char> &&data);

    enum force_yield_t {FORCE_YIELD, MAYBE_YIELD};
    void mailbox_read_coroutine(threadnum_t dest_thread,
                                raw_mailbox_t::id_t dest_mailbox_id,
                                std::vector<char> *stream_data,
                                int64_t stream_data_offset,
                                force_yield_t force_yield);
};

#endif /* RPC_MAILBOX_MAILBOX_HPP_ */
