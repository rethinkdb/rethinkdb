// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rpc/semilattice/semilattice_manager.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/versioned.hpp"
#include "logger.hpp"

#define MAX_OUTSTANDING_SEMILATTICE_WRITES 4

template<class metadata_t>
semilattice_manager_t<metadata_t>::semilattice_manager_t(
        connectivity_cluster_t *connectivity_cluster,
        connectivity_cluster_t::message_tag_t message_tag,
        const metadata_t &initial_metadata) :
    cluster_message_handler_t(connectivity_cluster, message_tag),
    root_view(boost::make_shared<root_view_t>(this)),
    metadata_version(0),
    metadata(initial_metadata),
    next_sync_from_query_id(0), next_sync_to_query_id(0),
    semaphore(MAX_OUTSTANDING_SEMILATTICE_WRITES),
    connection_change_subscription(
        get_connectivity_cluster()->get_connections(),
        std::bind(&semilattice_manager_t::on_connection_change, this, ph::_1, ph::_2),
        initial_call_t::NO)
{
    guarantee(get_connectivity_cluster()->get_connections()->get_all().empty());
}

template<class metadata_t>
semilattice_manager_t<metadata_t>::~semilattice_manager_t() THROWS_NOTHING {
    assert_thread();
    root_view->parent = nullptr;
}

template<class metadata_t>
boost::shared_ptr<semilattice_readwrite_view_t<metadata_t> > semilattice_manager_t<metadata_t>::get_root_view() {
    assert_thread();
    return root_view;
}

template<class metadata_t>
semilattice_manager_t<metadata_t>::root_view_t::root_view_t(semilattice_manager_t *p)
    : parent(p) {
    parent->assert_thread();
}

template<class metadata_t>
metadata_t semilattice_manager_t<metadata_t>::root_view_t::get() {
    guarantee(parent, "accessing `semilattice_manager_t` root view when cluster no longer exists");
    parent->assert_thread();
    return parent->metadata;
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::root_view_t::join(const metadata_t &added_metadata) {
    guarantee(parent, "accessing `semilattice_manager_t` root view when cluster no longer exists");
    parent->assert_thread();

    metadata_version_t new_version = ++parent->metadata_version;
    parent->join_metadata_locally(added_metadata);

    /* Distribute changes to all peers we can currently see. If we can't
    currently see a peer, that's OK; it will hear about the metadata change when
    it reconnects, via the `semilattice_manager_t`'s `on_connections_change()`
    handler. */
    auto_drainer_t::lock_t parent_keepalive(parent->drainers.get());

    for (const std::pair<peer_id_t, connectivity_cluster_t::connection_pair_t> &pair :
            parent->last_connections) {
        connectivity_cluster_t::connection_t *connection = pair.second.first;
        auto_drainer_t::lock_t connection_keepalive = pair.second.second;
        coro_t::spawn_sometime(
            [this, parent_keepalive /* important to capture */,
             connection, connection_keepalive /* important to capture */,
             new_version, added_metadata]() {
                metadata_writer_t writer(added_metadata, new_version);
                new_semaphore_in_line_t acq(&parent->semaphore, 1);
                acq.acquisition_signal()->wait();
                parent->get_connectivity_cluster()->send_message(connection,
                    connection_keepalive, parent->get_message_tag(), &writer);
            });
    }
}

static const char message_code_metadata = 'M';
static const char message_code_sync_from_query = 'F';
static const char message_code_sync_from_reply = 'f';
static const char message_code_sync_to_query = 'T';
static const char message_code_sync_to_reply = 't';

template <class metadata_t>
class semilattice_manager_t<metadata_t>::metadata_writer_t :
        public cluster_send_message_write_callback_t
{
public:
    metadata_writer_t(const metadata_t &_md, metadata_version_t _mdv) :
        md(_md), mdv(_mdv) { }

    void write(write_stream_t *stream) {
        write_message_t wm;
        // All cluster versions so far use a uint8_t code.
        uint8_t code = message_code_metadata;
        serialize_universal(&wm, code);
        serialize<cluster_version_t::CLUSTER>(&wm, md);
        serialize<cluster_version_t::CLUSTER>(&wm, mdv);
        int res = send_write_message(stream, &wm);
        if (res) { throw fake_archive_exc_t(); }
    }

#ifdef ENABLE_MESSAGE_PROFILER
    const char *message_profiler_tag() const {
        static const std::string tag =
            strprintf("semilattice<%s>.update", typeid(metadata_t).name());
        return tag.c_str();
    }
#endif

private:
    const metadata_t &md;
    metadata_version_t mdv;
};

template <class metadata_t>
class semilattice_manager_t<metadata_t>::sync_from_query_writer_t :
        public cluster_send_message_write_callback_t
{
public:
    explicit sync_from_query_writer_t(sync_from_query_id_t _query_id) :
        query_id(_query_id) { }

    void write(write_stream_t *stream) {
        write_message_t wm;
        // All cluster versions so far use a uint8_t code.
        uint8_t code = message_code_sync_from_query;
        serialize_universal(&wm, code);
        serialize<cluster_version_t::CLUSTER>(&wm, query_id);
        int res = send_write_message(stream, &wm);
        if (res) { throw fake_archive_exc_t(); }
    }

#ifdef ENABLE_MESSAGE_PROFILER
    const char *message_profiler_tag() const {
        static const std::string tag =
            strprintf("semilattice<%s>.sync_from", typeid(metadata_t).name());
        return tag.c_str();
    }
#endif

private:
    sync_from_query_id_t query_id;
};

template <class metadata_t>
class semilattice_manager_t<metadata_t>::sync_from_reply_writer_t :
        public cluster_send_message_write_callback_t
{
public:
    sync_from_reply_writer_t(sync_from_query_id_t _query_id, metadata_version_t _version) :
        query_id(_query_id), version(_version) { }

    void write(write_stream_t *stream) {
        write_message_t wm;
        // All cluster versions so far use a uint8_t code.
        uint8_t code = message_code_sync_from_reply;
        serialize_universal(&wm, code);
        serialize<cluster_version_t::CLUSTER>(&wm, query_id);
        serialize<cluster_version_t::CLUSTER>(&wm, version);
        int res = send_write_message(stream, &wm);
        if (res) { throw fake_archive_exc_t(); }
    }

#ifdef ENABLE_MESSAGE_PROFILER
    const char *message_profiler_tag() const {
        static const std::string tag =
            strprintf("semilattice<%s>.sync_from_reply", typeid(metadata_t).name());
        return tag.c_str();
    }
#endif

private:
    sync_from_query_id_t query_id;
    metadata_version_t version;
};

template <class metadata_t>
class semilattice_manager_t<metadata_t>::sync_to_query_writer_t :
        public cluster_send_message_write_callback_t
{
public:
    sync_to_query_writer_t(sync_to_query_id_t _query_id, metadata_version_t _version) :
        query_id(_query_id), version(_version) { }

    void write(write_stream_t *stream) {
        write_message_t wm;
        // All cluster versions so far use a uint8_t code.
        uint8_t code = message_code_sync_to_query;
        serialize_universal(&wm, code);
        serialize<cluster_version_t::CLUSTER>(&wm, query_id);
        serialize<cluster_version_t::CLUSTER>(&wm, version);
        int res = send_write_message(stream, &wm);
        if (res) { throw fake_archive_exc_t(); }
    }

#ifdef ENABLE_MESSAGE_PROFILER
    const char *message_profiler_tag() const {
        static const std::string tag =
            strprintf("semilattice<%s>.sync_to", typeid(metadata_t).name());
        return tag.c_str();
    }
#endif

private:
    sync_to_query_id_t query_id;
    metadata_version_t version;
};

template <class metadata_t>
class semilattice_manager_t<metadata_t>::sync_to_reply_writer_t :
        public cluster_send_message_write_callback_t
{
public:
    explicit sync_to_reply_writer_t(sync_to_query_id_t _query_id) :
        query_id(_query_id) { }

    void write(write_stream_t *stream) {
        write_message_t wm;
        // All cluster versions so far use a uint8_t code.
        uint8_t code = message_code_sync_to_reply;
        serialize_universal(&wm, code);
        serialize<cluster_version_t::CLUSTER>(&wm, query_id);
        int res = send_write_message(stream, &wm);
        if (res) { throw fake_archive_exc_t(); }
    }

#ifdef ENABLE_MESSAGE_PROFILER
    const char *message_profiler_tag() const {
        static const std::string tag =
            strprintf("semilattice<%s>.sync_to_reply", typeid(metadata_t).name());
        return tag.c_str();
    }
#endif

private:
    sync_to_query_id_t query_id;
};

template<class metadata_t>
void semilattice_manager_t<metadata_t>::root_view_t::sync_from(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
    guarantee(parent, "accessing `semilattice_manager_t` root view when cluster no longer exists");
    parent->assert_thread();

    /* Confirm that we are connected to the target peer */
    auto_drainer_t::lock_t connection_keepalive;
    connectivity_cluster_t::connection_t *connection =
        parent->get_connectivity_cluster()->
            get_connection(peer, &connection_keepalive);
    if (connection == nullptr) {
        throw sync_failed_exc_t();
    }

    /* Prepare so that we will be notified when the peer replies */
    sync_from_query_id_t query_id = parent->next_sync_from_query_id++;
    promise_t<metadata_version_t> response_cond;
    map_insertion_sentry_t<sync_from_query_id_t, promise_t<metadata_version_t> *> response_listener(&parent->sync_from_waiters, query_id, &response_cond);

    /* Send the sync-from message */
    sync_from_query_writer_t writer(query_id);
    {
        new_semaphore_in_line_t acq(&parent->semaphore, 1);
        wait_interruptible(acq.acquisition_signal(), interruptor);
        parent->get_connectivity_cluster()->send_message(connection,
            connection_keepalive, parent->get_message_tag(), &writer);
    }

    /* Wait until the peer replies, so we know what version to wait for */
    wait_any_t waiter(response_cond.get_ready_signal(),
                      connection_keepalive.get_drain_signal());
    wait_interruptible(&waiter, interruptor);   /* May throw `interrupted_exc_t` */
    if (connection_keepalive.get_drain_signal()->is_pulsed()) {
        throw sync_failed_exc_t();
    }

    /* Wait for that version */
    parent->wait_for_version_from_peer(peer, response_cond.wait(), interruptor);
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::root_view_t::sync_to(peer_id_t peer, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
    guarantee(parent, "accessing `semilattice_manager_t` root view when cluster no longer exists");
    parent->assert_thread();

    /* Confirm that we are connected to the target peer */
    auto_drainer_t::lock_t connection_keepalive;
    connectivity_cluster_t::connection_t *connection =
        parent->get_connectivity_cluster()->
            get_connection(peer, &connection_keepalive);
    if (connection == nullptr) {
        throw sync_failed_exc_t();
    }

    /* Prepare so that we will be notified when the peer replies */
    sync_to_query_id_t query_id = parent->next_sync_to_query_id++;
    cond_t response_cond;
    map_insertion_sentry_t<sync_to_query_id_t, cond_t *> response_listener(&parent->sync_to_waiters, query_id, &response_cond);

    /* Send the sync-to message */
    sync_to_query_writer_t writer(query_id, parent->metadata_version);
    {
        new_semaphore_in_line_t acq(&parent->semaphore, 1);
        wait_interruptible(acq.acquisition_signal(), interruptor);
        parent->get_connectivity_cluster()->send_message(
            connection, connection_keepalive, parent->get_message_tag(), &writer);
    }

    /* Wait until the peer replies; it won't reply until it's seen the version we told it
    to wait for. */
    wait_any_t waiter(&response_cond, connection_keepalive.get_drain_signal());
    wait_interruptible(&waiter, interruptor);   /* May throw `interrupted_exc_t` */
    if (connection_keepalive.get_drain_signal()->is_pulsed()) {
        throw sync_failed_exc_t();
    }
    guarantee(response_cond.is_pulsed());
}

template<class metadata_t>
publisher_t<std::function<void()> > *semilattice_manager_t<metadata_t>::root_view_t::get_publisher() {
    guarantee(parent, "accessing `semilattice_manager_t` root view when cluster no longer exists");
    parent->assert_thread();
    return parent->metadata_publisher.get_publisher();
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::on_message(
        connectivity_cluster_t::connection_t *connection,
        auto_drainer_t::lock_t connection_keepalive,
        read_stream_t *stream) {
    uint8_t code;
    {
        // All cluster versions so far use a uint8_t code for this.
        archive_result_t res = deserialize_universal(stream, &code);
        if (bad(res)) { throw fake_archive_exc_t(); }
    }

    peer_id_t sender = connection->get_peer_id();
    auto_drainer_t::lock_t this_keepalive(drainers.get());
    threadnum_t original_thread = get_thread_id();

    switch (code) {
        /* Another peer sent us a newer version of the metadata */
        case message_code_metadata: {
            metadata_t added_metadata;
            metadata_version_t change_version;
            {
                archive_result_t res =
                    deserialize<cluster_version_t::CLUSTER>(stream, &added_metadata);
                if (bad(res)) { throw fake_archive_exc_t(); }
                res = deserialize<cluster_version_t::CLUSTER>(stream, &change_version);
                if (bad(res)) { throw fake_archive_exc_t(); }
            }
            /* We have to spawn a new coroutine in order to go to the home thread */
            coro_t::spawn_sometime([this, this_keepalive /* important to capture */,
                    added_metadata, change_version, sender]() {
                on_thread_t thread_switcher(home_thread());
                /* This is the meat of the change */
                this->join_metadata_locally(added_metadata);
                /* Also notify anything that was waiting for us to reach this version */
                DEBUG_VAR mutex_assertion_t::acq_t acq(&this->peer_version_mutex);
                auto inserted = this->last_versions_seen.insert(
                    std::make_pair(sender, change_version));
                if (!inserted.second) {
                    inserted.first->second =
                        std::max(inserted.first->second, change_version);
                }
                for (auto it = version_waiters.begin();
                          it != version_waiters.end();
                          it++) {
                    if (it->first.first == sender &&
                            it->first.second <= change_version &&
                            !it->second->is_pulsed()) {
                        it->second->pulse();
                    }
                }
            });
            break;
        }
        /* A peer sent us a sync-from query. We must reply with our current metadata
        version. */
        case message_code_sync_from_query: {
            sync_from_query_id_t query_id;
            {
                archive_result_t res =
                    deserialize<cluster_version_t::CLUSTER>(stream, &query_id);
                if (bad(res)) { throw fake_archive_exc_t(); }
            }
            coro_t::spawn_sometime([this, this_keepalive /* important to capture */,
                    connection, connection_keepalive /* important to capture */,
                    query_id, original_thread]() {
                {
                    on_thread_t thread_switcher(home_thread());
                    sync_from_reply_writer_t writer(query_id, metadata_version);
                    new_semaphore_in_line_t acq(&this->semaphore, 1);
                    acq.acquisition_signal()->wait();
                    {
                        on_thread_t thread_switcher_2(original_thread);
                        get_connectivity_cluster()->send_message(connection,
                            connection_keepalive, get_message_tag(), &writer);
                    }
                }
            });
            break;
        }
        /* A peer sent us a sync-from reply. We must notify the coroutine that originated
        the query. */
        case message_code_sync_from_reply: {
            sync_from_query_id_t query_id;
            metadata_version_t version;
            {
                archive_result_t res =
                    deserialize<cluster_version_t::CLUSTER>(stream, &query_id);
                if (bad(res)) { throw fake_archive_exc_t(); }
                res = deserialize<cluster_version_t::CLUSTER>(stream, &version);
                if (bad(res)) { throw fake_archive_exc_t(); }
            }
            coro_t::spawn_sometime(
                [this, this_keepalive /* important to capture */, query_id, version]() {
                    on_thread_t thread_switcher(home_thread());
                    auto it = sync_from_waiters.find(query_id);
                    if (it != sync_from_waiters.end()) {
                        if (it->second->get_ready_signal()->is_pulsed()) {
                            logWRN("Got duplicate reply to a sync_from() call. TCP "
                                "checksum failure?");
                        } else {
                            it->second->pulse(version);
                        }
                    }
                });
            break;
        }
        /* A peer sent us a sync-to query. We must wait until we catch up to the given
        version, then reply. */
        case message_code_sync_to_query: {
            sync_from_query_id_t query_id;
            metadata_version_t version;
            {
                archive_result_t res =
                    deserialize<cluster_version_t::CLUSTER>(stream, &query_id);
                if (bad(res)) { throw fake_archive_exc_t(); }
                res = deserialize<cluster_version_t::CLUSTER>(stream, &version);
                if (bad(res)) { throw fake_archive_exc_t(); }
            }
            coro_t::spawn_sometime([this, this_keepalive /* important to capture */,
                    connection, connection_keepalive /* important to capture */,
                    query_id, version, original_thread]() {
                wait_any_t interruptor(this_keepalive.get_drain_signal(),
                                       connection_keepalive.get_drain_signal());
                cross_thread_signal_t interruptor2(&interruptor, home_thread());
                {
                    on_thread_t thread_switcher(home_thread());
                    try {
                        this->wait_for_version_from_peer(connection->get_peer_id(), version,
                            &interruptor2);
                    } catch (const interrupted_exc_t &) {
                        return;
                    } catch (const sync_failed_exc_t &) {
                        return;
                    }
                    sync_to_reply_writer_t writer(query_id);
                    new_semaphore_in_line_t acq(&this->semaphore, 1);
                    acq.acquisition_signal()->wait();
                    {
                        on_thread_t thread_switcher_2(original_thread);
                        get_connectivity_cluster()->send_message(connection,
                            connection_keepalive, get_message_tag(), &writer);
                    }
                }
            });
            break;
        }
        /* A peer sent us a sync-to reply. We must notify the coroutine that spawned the
        query. */
        case message_code_sync_to_reply: {
            sync_from_query_id_t query_id;
            {
                archive_result_t res =
                    deserialize<cluster_version_t::CLUSTER>(stream, &query_id);
                if (bad(res)) { throw fake_archive_exc_t(); }
            }
            coro_t::spawn_sometime(
                [this, this_keepalive /* important to capture */, query_id]() {
                    on_thread_t thread_switcher(home_thread());
                    auto it = sync_to_waiters.find(query_id);
                    if (it != sync_to_waiters.end()) {
                        if (it->second->is_pulsed()) {
                            logWRN("Got duplicate reply to a sync_to() call. TCP "
                                "checksum failure?");
                        } else {
                            it->second->pulse();
                        }
                    }
                });
            break;
        }
        default: {
            /* We don't try to tolerate garbage on the wire. The network layer had
            better not corrupt our messages. If we need to, we'll add more
            checksums. */
            crash("Unexpected semilattice message code: %u", code);
        }
    }
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::on_connection_change(
        const peer_id_t &peer_id,
        const connectivity_cluster_t::connection_pair_t *pair) {
    if (pair != nullptr && last_connections.count(peer_id) == 0) {
        connectivity_cluster_t::connection_t *connection = pair->first;
        auto_drainer_t::lock_t connection_keepalive = pair->second;
        last_connections.insert(std::make_pair(peer_id, *pair));
        auto_drainer_t::lock_t this_keepalive(drainers.get());
        coro_t::spawn_sometime([this, this_keepalive /* important to capture */,
                connection, connection_keepalive /* important to capture */]() {
            metadata_writer_t writer(metadata, metadata_version);
            new_semaphore_in_line_t acq(&this->semaphore, 1);
            acq.acquisition_signal()->wait();
            get_connectivity_cluster()->send_message(connection,
                connection_keepalive, get_message_tag(), &writer);
        });
    }
    if (pair == nullptr && last_connections.count(peer_id) == 1) {
        last_connections.erase(peer_id);
    }
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::join_metadata_locally(metadata_t added_metadata) {
    assert_thread();
    DEBUG_VAR rwi_lock_assertion_t::write_acq_t acq(&metadata_mutex);
    semilattice_join(&metadata, added_metadata);
    metadata_publisher.publish(
        [](const std::function<void()> &fun) {
            fun();
        });
}

template<class metadata_t>
void semilattice_manager_t<metadata_t>::wait_for_version_from_peer(peer_id_t peer, metadata_version_t version, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, sync_failed_exc_t) {
    assert_thread();
    mutex_assertion_t::acq_t acq(&peer_version_mutex);
    typename std::map<peer_id_t, metadata_version_t>::iterator it = last_versions_seen.find(peer);
    if (it != last_versions_seen.end() && it->second >= version) {
        return;
    }

    auto_drainer_t::lock_t connection_keepalive;
    connectivity_cluster_t::connection_t *connection =
        get_connectivity_cluster()->
            get_connection(peer, &connection_keepalive);
    if (connection == nullptr) {
        throw sync_failed_exc_t();
    }

    cond_t caught_up;
    multimap_insertion_sentry_t<std::pair<peer_id_t, metadata_version_t>, cond_t *> insertion_sentry(
        &version_waiters, std::make_pair(peer, version), &caught_up);

    acq.reset();
    wait_any_t waiter(&caught_up, connection_keepalive.get_drain_signal());
    wait_interruptible(&waiter, interruptor);
    if (connection_keepalive.get_drain_signal()->is_pulsed()) {
        throw sync_failed_exc_t();
    }
    guarantee(caught_up.is_pulsed());
}

