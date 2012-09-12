#include "clustering/administration/persist.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include "arch/runtime/thread_pool.hpp"
#include "buffer_cache/blob.hpp"
#include "containers/archive/string_stream.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "serializer/config.hpp"

namespace metadata_persistence {

struct metadata_superblock_t {
    block_magic_t magic;

    machine_id_t machine_id;

    static const int METADATA_BLOB_MAXREFLEN = 1500;
    char metadata_blob[METADATA_BLOB_MAXREFLEN];

    static const int BRANCH_HISTORY_BLOB_MAXREFLEN = 500;
    char dummy_branch_history_blob[BRANCH_HISTORY_BLOB_MAXREFLEN];
    char memcached_branch_history_blob[BRANCH_HISTORY_BLOB_MAXREFLEN];
    char rdb_branch_history_blob[BRANCH_HISTORY_BLOB_MAXREFLEN];

    static const block_magic_t expected_magic;
};

/* Etymology: (R)ethink(D)B (m)eta(d)ata */
const block_magic_t metadata_superblock_t::expected_magic = { { 'R', 'D', 'm', 'd' } };

template <class T>
static void write_blob(transaction_t *txn, char *ref, int maxreflen, const T &value) {
    write_message_t msg;
    msg << value;
    intrusive_list_t<write_buffer_t> *buffers = msg.unsafe_expose_buffers();
    size_t slen = 0;
    for (write_buffer_t *p = buffers->head(); p != NULL; p = buffers->next(p)) {
        slen += p->size;
    }
    std::string str;
    str.reserve(slen);
    for (write_buffer_t *p = buffers->head(); p != NULL; p = buffers->next(p)) {
        str.append(p->data, p->size);
    }
    guarantee(str.size() == slen);
    blob_t blob(ref, maxreflen);
    blob.clear(txn);
    blob.append_region(txn, str.size());
    blob.write_from_string(str, txn, 0);
    guarantee(blob.valuesize() == static_cast<int64_t>(slen));
}

template<class T>
static void read_blob(transaction_t *txn, const char *ref, int maxreflen, T *value_out) {
    blob_t blob(const_cast<char *>(ref), maxreflen);
    std::string str = blob.read_to_string(txn, 0, blob.valuesize());
    read_string_stream_t ss(str);
    int res = deserialize(&ss, value_out);
    guarantee(res == 0);
}

persistent_file_t::persistent_file_t(io_backender_t *io_backender, const std::string &filename, perfmon_collection_t *perfmon_parent) {
    construct_serializer_and_cache(io_backender, false, filename, perfmon_parent);
    construct_branch_history_managers(false);
}

persistent_file_t::persistent_file_t(io_backender_t *io_backender, const std::string& filename, perfmon_collection_t *perfmon_parent, const machine_id_t &machine_id, const cluster_semilattice_metadata_t &initial_metadata) {
    construct_serializer_and_cache(io_backender, true, filename, perfmon_parent);

    transaction_t txn(cache.get(), rwi_write, 1, repli_timestamp_t::distant_past, order_token_t::ignore);
    buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_write);
    metadata_superblock_t *sb = static_cast<metadata_superblock_t *>(superblock.get_data_major_write());

    bzero(sb, cache->get_block_size().value());
    sb->magic = metadata_superblock_t::expected_magic;
    sb->machine_id = machine_id;
    write_blob(&txn, sb->metadata_blob, metadata_superblock_t::METADATA_BLOB_MAXREFLEN, initial_metadata);
    write_blob(&txn, sb->dummy_branch_history_blob, metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN, branch_history_t<mock::dummy_protocol_t>());
    write_blob(&txn, sb->memcached_branch_history_blob, metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN, branch_history_t<memcached_protocol_t>());
    write_blob(&txn, sb->rdb_branch_history_blob, metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN, branch_history_t<rdb_protocol_t>());

    construct_branch_history_managers(true);
}

persistent_file_t::~persistent_file_t() {
    /* This destructor is defined here so that the compiler won't try to
    generate a destructor in the header file. In the header file, it can't see
    the definition of `persistent_branch_history_manager_t`, so any attempt to
    generate the destructor will fail. */
}

machine_id_t persistent_file_t::read_machine_id() {
    transaction_t txn(cache.get(), rwi_read, 0, repli_timestamp_t::distant_past, order_token_t::ignore);
    buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_read);
    const metadata_superblock_t *sb = static_cast<const metadata_superblock_t *>(superblock.get_data_read());
    return sb->machine_id;
}

cluster_semilattice_metadata_t persistent_file_t::read_metadata() {
    transaction_t txn(cache.get(), rwi_read, 0, repli_timestamp_t::distant_past, order_token_t::ignore);
    buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_read);
    const metadata_superblock_t *sb = static_cast<const metadata_superblock_t *>(superblock.get_data_read());
    cluster_semilattice_metadata_t metadata;
    read_blob(&txn, sb->metadata_blob, metadata_superblock_t::METADATA_BLOB_MAXREFLEN, &metadata);
    return metadata;
}

void persistent_file_t::update_metadata(const cluster_semilattice_metadata_t &metadata) {
    transaction_t txn(cache.get(), rwi_write, 1, repli_timestamp_t::distant_past, order_token_t::ignore);
    buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_write);
    metadata_superblock_t *sb = static_cast<metadata_superblock_t *>(superblock.get_data_major_write());
    write_blob(&txn, sb->metadata_blob, metadata_superblock_t::METADATA_BLOB_MAXREFLEN, metadata);
}

template <class protocol_t>
class persistent_file_t::persistent_branch_history_manager_t : public branch_history_manager_t<protocol_t> {
public:
    persistent_branch_history_manager_t(persistent_file_t *p, char (metadata_superblock_t::*fn)[metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN], bool create) :
        parent(p), field_name(fn)
    {
        /* If we're not creating, we have to load the existing branch history
        database from disk */
        if (!create) {
            transaction_t txn(parent->cache.get(), rwi_read, 0, repli_timestamp_t::distant_past, order_token_t::ignore);
            buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_read);
            const metadata_superblock_t *sb = static_cast<const metadata_superblock_t *>(superblock.get_data_read());
            read_blob(&txn, sb->*field_name, metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN, &bh);
        }
    }

    branch_birth_certificate_t<protocol_t> get_branch(branch_id_t branch) THROWS_NOTHING {
        home_thread_mixin_t::assert_thread();
        typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::const_iterator it = bh.branches.find(branch);
        guarantee(it != bh.branches.end(), "no such branch");
        return it->second;
    }

    void create_branch(branch_id_t branch_id, const branch_birth_certificate_t<protocol_t> &bc, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        guarantee(bh.branches.find(branch_id) == bh.branches.end());
        bh.branches[branch_id] = bc;
        flush(interruptor);
    }

    void export_branch_history(branch_id_t branch, branch_history_t<protocol_t> *out) THROWS_NOTHING {
        home_thread_mixin_t::assert_thread();
        std::set<branch_id_t> to_process;
        if (out->branches.count(branch) == 0) {
            to_process.insert(branch);
        }
        while (!to_process.empty()) {
            branch_id_t next = *to_process.begin();
            to_process.erase(next);
            branch_birth_certificate_t<protocol_t> bc = get_branch(next);
            guarantee(out->branches.count(next) == 0);
            out->branches[next] = bc;
            for (typename region_map_t<protocol_t, version_range_t>::const_iterator it = bc.origin.begin(); it != bc.origin.end(); it++) {
                if (!it->second.latest.branch.is_nil() && out->branches.count(it->second.latest.branch) == 0) {
                    to_process.insert(it->second.latest.branch);
                }
            }
        }
    }

    void import_branch_history(const branch_history_t<protocol_t> &new_records, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        for (typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::const_iterator it = new_records.branches.begin(); it != new_records.branches.end(); it++) {
            bh.branches.insert(std::make_pair(it->first, it->second));
        }
        flush(interruptor);
    }

private:
    void flush(UNUSED signal_t *interruptor) {
        transaction_t txn(parent->cache.get(), rwi_write, 1, repli_timestamp_t::distant_past, order_token_t::ignore);
        buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_write);
        metadata_superblock_t *sb = static_cast<metadata_superblock_t *>(superblock.get_data_major_write());
        write_blob(&txn, sb->*field_name, metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN, bh);
    }

    persistent_file_t *parent;
    char (metadata_superblock_t::*field_name)[metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN];
    branch_history_t<protocol_t> bh;
};

/* These must be defined when the definition of
`persistent_branch_history_manager_t` is in scope, so that the compiler knows
that `persistent_branch_history_manager_t *` can be implicitly cast to
`branch_history_manager_t *`. */

branch_history_manager_t<mock::dummy_protocol_t> *persistent_file_t::get_dummy_branch_history_manager() {
    return dummy_branch_history_manager.get();
}

branch_history_manager_t<memcached_protocol_t> *persistent_file_t::get_memcached_branch_history_manager() {
    return memcached_branch_history_manager.get();
}

branch_history_manager_t<rdb_protocol_t> *persistent_file_t::get_rdb_branch_history_manager() {
    return rdb_branch_history_manager.get();
}

void persistent_file_t::construct_serializer_and_cache(io_backender_t *io_backender, const bool create, const std::string &filename, perfmon_collection_t *const perfmon_parent) {
    standard_serializer_t::dynamic_config_t serializer_dynamic_config;

    if (create) {
        standard_serializer_t::create(io_backender,
                                      standard_serializer_t::private_dynamic_config_t(filename),
                                      standard_serializer_t::static_config_t());
    }

    serializer.init(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                              io_backender,
                                              standard_serializer_t::private_dynamic_config_t(filename),
                                              perfmon_parent));

    if (create) {
        mirrored_cache_static_config_t cache_static_config;
        cache_t::create(serializer.get(), &cache_static_config);
    }

    cache_dynamic_config.wait_for_flush = true;         // flush to disk immediately on change
    cache_dynamic_config.flush_waiting_threshold = 0;
    cache_dynamic_config.max_size = MEGABYTE;
    cache_dynamic_config.max_dirty_size = MEGABYTE / 2;
    cache.init(new cache_t(serializer.get(), &cache_dynamic_config, perfmon_parent));
}

void persistent_file_t::construct_branch_history_managers(bool create) {
    dummy_branch_history_manager.init(new persistent_branch_history_manager_t<mock::dummy_protocol_t>(
        this, &metadata_superblock_t::dummy_branch_history_blob, create));
    memcached_branch_history_manager.init(new persistent_branch_history_manager_t<memcached_protocol_t>(
        this, &metadata_superblock_t::memcached_branch_history_blob, create));
    rdb_branch_history_manager.init(new persistent_branch_history_manager_t<rdb_protocol_t>(
        this, &metadata_superblock_t::rdb_branch_history_blob, create));
}

semilattice_watching_persister_t::semilattice_watching_persister_t(
        persistent_file_t *persistent_file_,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > v) :
    persistent_file(persistent_file_), view(v),
    flush_again(new cond_t),
    subs(boost::bind(&semilattice_watching_persister_t::on_change, this), v)
{
    coro_t::spawn_sometime(boost::bind(&semilattice_watching_persister_t::dump_loop, this, auto_drainer_t::lock_t(&drainer)));
}

void semilattice_watching_persister_t::dump_loop(auto_drainer_t::lock_t keepalive) {
    try {
        for (;;) {
            persistent_file->update_metadata(view->get());
            {
                wait_any_t c(flush_again.get(), &stop);
                wait_interruptible(&c, keepalive.get_drain_signal());
            }
            if (flush_again->is_pulsed()) {
                scoped_ptr_t<cond_t> tmp(new cond_t);
                flush_again.swap(tmp);
            } else {
                break;
            }
        }
    } catch (interrupted_exc_t) {
        /* ignore */
    }
    stopped.pulse();
}

void semilattice_watching_persister_t::on_change() {
    if (!flush_again->is_pulsed()) {
        flush_again->pulse();
    }
}

}  // namespace metadata_persistence
