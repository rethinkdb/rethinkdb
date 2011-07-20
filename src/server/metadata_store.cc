#include "server/metadata_store.hpp"

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "btree/rget.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/signal.hpp"
#include "concurrency/side_coro.hpp"
#include "concurrency/pmap.hpp"
#include "containers/iterators.hpp"
#include "db_thread_info.hpp"
#include "replication/backfill.hpp"
#include "replication/master.hpp"
#include "server/cmd_args.hpp"
#include "arch/timing.hpp"
#include "stats/persist.hpp"
#include "cmd_args.hpp"


static void co_persist_stats(btree_metadata_store_t *store, signal_t *shutdown);

void btree_metadata_store_t::create(btree_key_value_store_dynamic_config_t *dynamic_config,
                                     btree_key_value_store_static_config_t *static_config) {

    int n_files = dynamic_config->serializer_private.size();
    rassert(n_files > 0);
    rassert(n_files <= MAX_SERIALIZERS);
    guarantee(n_files == 1, "Metadata must only use a single file");

    /* Create, initialize, & shutdown metadata serializer */
    standard_serializer_t::create(&dynamic_config->serializer,
                                  &dynamic_config->serializer_private[0],
                                  &static_config->serializer);
    standard_serializer_t *metadata_serializer;
    btree_store_helpers::create_existing_serializer(&metadata_serializer, n_files,
                               &dynamic_config->serializer, &dynamic_config->serializer_private[0]);
    btree_store_helpers::prep_serializer(metadata_serializer, &static_config->cache, n_files);
    btree_store_helpers::destroy_serializer(metadata_serializer);
}

btree_metadata_store_t::btree_metadata_store_t(const btree_key_value_store_dynamic_config_t &dynamic_config) {

    /* Keep a local copy of the dynamic configuration */
    store_dynamic_config = dynamic_config;

    /* Start data shard serializer */
    int n_files = store_dynamic_config.serializer_private.size();
    guarantee(n_files == 1, "Metadata must only use a single file");

    btree_static_config.n_slices = 1;

    /* Start metadata serializer & load its btree */
    btree_store_helpers::create_existing_serializer(&metadata_serializer, n_files,
                               &store_dynamic_config.serializer,
                               &store_dynamic_config.serializer_private[0]); // TODO! Clean up configuration (metadata_store should have its own configuration types)
    btree_store_helpers::create_existing_shard(&metadata_shard, btree_static_config.n_slices,
                          metadata_serializer, &store_dynamic_config.cache, store_dynamic_config.total_delete_queue_limit);

    // Unpersist stats & create the stat persistence coro
    // TODO (rntz) should this really be in the constructor? what if it errors?
    // But how else can I ensure the first unpersist happens before the first persist?
    persistent_stat_t::unpersist_all(this);
    stat_persistence_side_coro_ptr =
        new side_coro_handler_t(boost::bind(&co_persist_stats, this, _1));
}

btree_metadata_store_t::~btree_metadata_store_t() {
    // make sure side coro finishes so we're done with the metadata shard
    delete stat_persistence_side_coro_ptr;

    /* Shut down btree */
    {
        on_thread_t thread_switcher(metadata_shard->home_thread());
        delete metadata_shard;
    }

    /* Shut down serializer */
    btree_store_helpers::destroy_serializer(metadata_serializer);
}

// metadata interface
static store_key_t key_from_string(const std::string &key) {
    guarantee(key.size() <= MAX_KEY_SIZE);
    store_key_t sk;
    bool b = str_to_key(key.data(), &sk);
    rassert(b, "str_to_key on key of length < MAX_KEY_SIZE failed");
    return sk;
    (void) b;                   // avoid unused variable warning on release build
}

bool btree_metadata_store_t::get_meta(const std::string &key, std::string *out) {
    store_key_t sk = key_from_string(key);
    // TODO (rntz) should we be worrying about order tokens?
    get_result_t res = metadata_shard->get(sk, order_token_t::ignore);
    // This should only be tripped if a gated store was involved, which it wasn't.
    guarantee(!res.is_not_allowed);
    if (!res.value) return false;

    // Get the data & copy it into the outstring
    const const_buffer_group_t *bufs = res.value->get_data_as_buffers();
    out->assign("");
    out->reserve(bufs->get_size());
    size_t nbufs = bufs->num_buffers();
    for (unsigned i = 0; i < nbufs; ++i) {
        const_buffer_group_t::buffer_t buf = bufs->get_buffer(i);
        out->append(reinterpret_cast<const char *>(buf.data), buf.size);
    }
    return true;
}

void btree_metadata_store_t::set_meta(const std::string &key, const std::string &value) {
    store_key_t sk = key_from_string(key);
    boost::shared_ptr<buffered_data_provider_t>
        datap(new buffered_data_provider_t(value.data(), value.size()));

    // TODO (rntz) code dup with run_storage_command :/
    mcflags_t mcflags = 0;      // default, no flags
    // TODO (rntz) what if it's a large value, and needs the LARGE_VALUE flag? how do we determine this?
    exptime_t exptime = 0;      // indicates never expiring

    set_result_t res = metadata_shard->sarc(sk, datap, mcflags, exptime,
        add_policy_yes, replace_policy_yes, // "set" semantics: insert if not present, overwrite if present
        NO_CAS_SUPPLIED, // not a CAS operation
        // TODO (rntz) do we need to worry about ordering?
        order_token_t::ignore);

    // TODO (rntz) consider error conditions more thoroughly
    // For now, we assume "too large" or "not allowed" can't happen.
    guarantee(res == sr_stored);
}

static void co_persist_stats(btree_metadata_store_t *store, signal_t *shutdown) {
    while (!shutdown->is_pulsed()) {
        signal_timer_t timer(STAT_PERSIST_FREQUENCY_MS);
        cond_t wakeup;
        cond_link_t link_shutdown(shutdown, &wakeup);
        cond_link_t link_timer(&timer, &wakeup);
        wakeup.wait();

        // Persist stats
        persistent_stat_t::persist_all(store);
    }
}
