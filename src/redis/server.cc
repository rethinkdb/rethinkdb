#ifndef NO_REDIS
#include "redis/server.hpp"

#include <iostream>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "db_thread_info.hpp"
#include "perfmon.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"

// TODO: Get rid of these unittest inclusions (or get rid of this file).
#include "unittest/unittest_utils.hpp"
#include "unittest/dummy_namespace_interface.hpp"

// unittests::temp_file_t is only defined in the unitest binary so can't be used here,
// thus this hack, justifiable only because we can expect the dummy_namespace_interface_t
// used here to go away soon.

struct temp_file_t2 {
    explicit temp_file_t2(const char *tmpl) {
        size_t len = strlen(tmpl);
        filename = new char[len+1];
        memcpy(filename, tmpl, len+1);
        int fd = mkstemp(filename);
        guarantee_err(fd != -1, "Couldn't create a temporary file");
        close(fd);
    }

    ~temp_file_t2() {
        unlink(filename);
        delete [] filename;
    }

    const char *name() { return filename; }
    char *filename;
};

redis_listener_t::redis_listener_t(int port) :
    next_thread(0)
{
    // Set up redis stack

    std::vector<redis_protocol_t::region_t> shards;
    key_range_t key_range1(key_range_t::none, store_key_t(""),  key_range_t::open, store_key_t("zzzzzzzzzz"));
    //key_range_t key_range2(key_range_t::none, store_key_t("n"),  key_range_t::open, store_key_t(""));
    shards.push_back(key_range1);
    //shards.push_back(key_range2);

    temp_file_t2 db_file("/tmp/rdb_unittest.XXXXXX");

    /* Set up serializer */

    standard_serializer_t::create(
        standard_serializer_t::dynamic_config_t(),
        standard_serializer_t::private_dynamic_config_t(db_file.name()),
        standard_serializer_t::static_config_t()
        );

    standard_serializer_t *serializer = new standard_serializer_t(
        /* Extra parentheses are necessary so C++ doesn't interpret this as
        a declaration of a function called `serializer`. WTF, C++? */
        (standard_serializer_t::dynamic_config_t()),
        standard_serializer_t::private_dynamic_config_t(db_file.name())
        );

    /* Set up multiplexer */

    std::vector<standard_serializer_t *> multiplexer_files;
    multiplexer_files.push_back(serializer);

    serializer_multiplexer_t::create(multiplexer_files, shards.size());

    serializer_multiplexer_t *multiplexer = new serializer_multiplexer_t(multiplexer_files);
    rassert(multiplexer->proxies.size() == shards.size());

    /* Set up caches, btrees, and stores */

    mirrored_cache_config_t *cache_dynamic_config = new mirrored_cache_config_t;
    std::vector<boost::shared_ptr<store_view_t<redis_protocol_t> > > stores;
    for (int i = 0; i < (int)shards.size(); i++) {
        mirrored_cache_static_config_t cache_static_config;
        cache_t::create(multiplexer->proxies[i], &cache_static_config);
        cache_t *cache = new cache_t(multiplexer->proxies[i], cache_dynamic_config);
        btree_slice_t::create(cache);
        btree_slice_t *btree = new btree_slice_t(cache);
        stores.push_back(boost::make_shared<dummy_redis_store_view_t>(shards[i], btree));
    }

    /* Set up namespace interface */
    redis_interface = new unittest::dummy_namespace_interface_t<redis_protocol_t>(shards, stores);

    // Listen on port
    tcp_listener.reset(new tcp_listener_t(port, boost::bind(&redis_listener_t::handle, this, _1)));
}

redis_listener_t::~redis_listener_t() {

    // Stop accepting new connections
    tcp_listener.reset();

    // Interrupt existing connections
    pulse_to_begin_shutdown.pulse();

    // Wait for existing connections to finish shutting down
    active_connection_drain_semaphore.drain();
}

static void close_conn_if_open(tcp_conn_t *conn) {
    if (conn->is_read_open()) conn->shutdown_read();
}
 
void redis_listener_t::handle(boost::scoped_ptr<nascent_tcp_conn_t> &nconn) {
    assert_thread();

    drain_semaphore_t::lock_t dont_shut_down_yet(&active_connection_drain_semaphore);

    /* We will switch to another thread so there isn't too much load on the thread
    where the `memcache_listener_t` lives */

    // TODO We don't switch threads here to alieviate threading issues. The namespace_interface code will deal with
    // said threading issues eventually.
    int chosen_thread = get_thread_id();//(next_thread++) % get_num_db_threads();

    /* Construct a cross-thread watcher so we will get notified on `chosen_thread`
    when a shutdown command is delivered on the main thread. */
    cross_thread_signal_t signal_transfer(&pulse_to_begin_shutdown, chosen_thread);

    on_thread_t thread_switcher(chosen_thread);
    boost::scoped_ptr<tcp_conn_t> conn;
    nconn->ennervate(conn);

    /* Set up an object that will close the network connection when a shutdown signal
    is delivered */
    signal_t::subscription_t conn_closer(
        boost::bind(&close_conn_if_open, conn.get()),
        &signal_transfer);

    /* `serve_redis()` will continuously serve redis queries on the given conn
    until the connection is closed. */
    serve_redis(conn.get(), redis_interface);
}
#endif //#ifndef NO_REDIS
