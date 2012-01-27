#include "server/metadata_store.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/timing.hpp"
#include "btree/rget.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/signal.hpp"
#include "concurrency/side_coro.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/iterators.hpp"
#include "db_thread_info.hpp"
#include "replication/backfill.hpp"
#include "replication/master.hpp"
#include "server/cmd_args.hpp"
#include "serializer/config.hpp"
#include "stats/persist.hpp"


static void co_persist_stats(btree_metadata_store_t *store, signal_t *shutdown);

void btree_metadata_store_t::create(btree_key_value_store_dynamic_config_t *dynamic_config,
				    btree_key_value_store_static_config_t *static_config) {

    int n_files = dynamic_config->serializer_private.size();
    rassert(n_files > 0);
    rassert(n_files <= MAX_SERIALIZERS);
    guarantee(n_files == 1, "Metadata must only use a single file");

    btree_key_value_store_t::create(dynamic_config, static_config);
}

btree_metadata_store_t::btree_metadata_store_t(const btree_key_value_store_dynamic_config_t& dynamic_config)
    : stat_persistence_side_coro_ptr(NULL) {

    guarantee(dynamic_config.serializer_private.size() == 1, "Metadata must only use a single file");

    store_.reset(new btree_key_value_store_t(dynamic_config));

    seq_group_.reset(new sequence_group_t(store_->btree_static_config.n_slices));

    // Unpersist stats & create the stat persistence coro
    persistent_stat_t::unpersist_all(this);
    stat_persistence_side_coro_ptr =
        new side_coro_handler_t(boost::bind(&co_persist_stats, this, _1));
}

btree_metadata_store_t::~btree_metadata_store_t() {
    // make sure side coro finishes so we're done with the metadata shard
    delete stat_persistence_side_coro_ptr;
}

static store_key_t key_from_string(const std::string& key) {
    guarantee(key.size() <= MAX_KEY_SIZE);
    return store_key_t(key);
}

bool btree_metadata_store_t::get_meta(const std::string &key, std::string *out) {
    store_key_t sk = key_from_string(key);

    get_result_t res = store_->get(sk, seq_group_.get(), order_token_t::ignore);

    // This should only be tripped if a gated store was involved, which it wasn't.
    guarantee(!res.is_not_allowed);

    if (!res.value) {
	return false;
    }

    // Get the data and copy it into *out.
    out->assign(res.value->buf(), res.value->size());
    return true;
}

void btree_metadata_store_t::set_meta(const std::string& key, const std::string& value) {
    store_key_t sk = key_from_string(key);
    boost::intrusive_ptr<data_buffer_t> datap = data_buffer_t::create(value.size());
    memcpy(datap->buf(), value.data(), datap->size());

    // TODO (rntz) code dup with run_storage_command :/
    mcflags_t mcflags = 0;
    exptime_t exptime = 0;

    set_result_t res = store_->sarc(seq_group_.get(), sk, datap, mcflags, exptime,
        add_policy_yes, replace_policy_yes,
        NO_CAS_SUPPLIED,
        order_token_t::ignore);

    // TODO (rntz) consider error conditions more thoroughly
    // For now, we assume "too large" or "not allowed" can't happen.
    guarantee(res == sr_stored);
}

static void co_persist_stats(btree_metadata_store_t *store, signal_t *shutdown) {
    while (!shutdown->is_pulsed()) {
        signal_timer_t timer(STAT_PERSIST_FREQUENCY_MS);
        wait_any_t waiter(shutdown, &timer);
        waiter.wait_lazily_unordered();

        // Persist stats
        persistent_stat_t::persist_all(store);
    }
}
