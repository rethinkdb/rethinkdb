// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/persist.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include "arch/runtime/thread_pool.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/blob.hpp"
#include "buffer_cache/alt/cache_balancer.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "serializer/config.hpp"

namespace metadata_persistence {

struct auth_metadata_superblock_t {
    block_magic_t magic;

    static const int METADATA_BLOB_MAXREFLEN = 1500;
    char metadata_blob[METADATA_BLOB_MAXREFLEN];

};

// This version is used for the cluster_persistent_file_t
struct cluster_metadata_superblock_t {
    block_magic_t magic;

    machine_id_t machine_id;

    static const int METADATA_BLOB_MAXREFLEN = 1500;
    char metadata_blob[METADATA_BLOB_MAXREFLEN];

    static const int BRANCH_HISTORY_BLOB_MAXREFLEN = 500;
    char dummy_branch_history_blob[BRANCH_HISTORY_BLOB_MAXREFLEN];
    char memcached_branch_history_blob[BRANCH_HISTORY_BLOB_MAXREFLEN];
    char rdb_branch_history_blob[BRANCH_HISTORY_BLOB_MAXREFLEN];
};

/* Etymology: (R)ethink(D)B (m)eta(d)ata */
const block_magic_t expected_magic = { { 'R', 'D', 'm', 'd' } };

template <class T>
static void write_blob(buf_parent_t parent, char *ref, int maxreflen,
                       const T &value) {
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
    blob_t blob(parent.cache()->get_block_size(), ref, maxreflen);
    blob.clear(parent);
    blob.append_region(parent, str.size());
    blob.write_from_string(str, parent, 0);
    guarantee(blob.valuesize() == static_cast<int64_t>(slen));
}

template<class T>
static void read_blob(buf_parent_t parent, const char *ref, int maxreflen,
                      T *value_out) {
    blob_t blob(parent.cache()->get_block_size(),
                     const_cast<char *>(ref), maxreflen);
    blob_acq_t acq_group;
    buffer_group_t group;
    blob.expose_all(parent, access_t::read, &group, &acq_group);
    buffer_group_read_stream_t ss(const_view(&group));
    archive_result_t res = deserialize(&ss, value_out);
    guarantee_deserialization(res, "T (template code)");
}

template <class metadata_t>
persistent_file_t<metadata_t>::persistent_file_t(io_backender_t *io_backender,
                                                 const serializer_filepath_t &filename,
                                                 perfmon_collection_t *perfmon_parent,
                                                 bool create) {
    filepath_file_opener_t file_opener(filename, io_backender);
    construct_serializer_and_cache(create, &file_opener, perfmon_parent);

    if (create) {
        file_opener.move_serializer_file_to_permanent_location();
    }
}

template <class metadata_t>
persistent_file_t<metadata_t>::~persistent_file_t() {
    /* This destructor is defined here so that the compiler won't try to
    generate a destructor in the header file. In the header file, it can't see
    the definition of `persistent_branch_history_manager_t`, so any attempt to
    generate the destructor will fail. */
}

template <class metadata_t>
block_size_t persistent_file_t<metadata_t>::get_cache_block_size() const {
    return cache->get_block_size();
}

template <class metadata_t>
void persistent_file_t<metadata_t>::construct_serializer_and_cache(const bool create,
                                                                   serializer_file_opener_t *file_opener,
                                                                   perfmon_collection_t *const perfmon_parent) {
    standard_serializer_t::dynamic_config_t serializer_dynamic_config;

    if (create) {
        standard_serializer_t::create(file_opener,
                                      standard_serializer_t::static_config_t());
    }

    serializer.init(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                              file_opener,
                                              perfmon_parent));

    if (!serializer->coop_lock_and_check()) {
        throw file_in_use_exc_t();
    }

    {
        balancer.init(new dummy_cache_balancer_t(MEGABYTE));
        cache.init(new cache_t(serializer.get(), balancer.get(), perfmon_parent));
        cache_conn.init(new cache_conn_t(cache.get()));
    }

    if (create) {
        object_buffer_t<txn_t> txn;
        get_write_transaction(&txn);
        buf_lock_t superblock(txn.get(), SUPERBLOCK_ID, alt_create_t::create);
        buf_write_t sb_write(&superblock);
        void *sb_data = sb_write.get_data_write(cache->max_block_size().value());
        memset(sb_data, 0, cache->max_block_size().value());
    }
}

template <class metadata_t>
void persistent_file_t<metadata_t>::get_write_transaction(object_buffer_t<txn_t> *txn_out) {
    txn_out->create(cache_conn.get(),
                    write_durability_t::HARD,
                    repli_timestamp_t::distant_past,
                    1);
}

template <class metadata_t>
void persistent_file_t<metadata_t>::get_read_transaction(object_buffer_t<txn_t> *txn_out) {
    txn_out->create(cache_conn.get(),
                    read_access_t::read);
}

auth_persistent_file_t::auth_persistent_file_t(io_backender_t *io_backender,
                                               const serializer_filepath_t &filename,
                                               perfmon_collection_t *perfmon_parent) :
    persistent_file_t<auth_semilattice_metadata_t>(io_backender, filename, perfmon_parent, false) {
    // Do nothing
}

auth_persistent_file_t::auth_persistent_file_t(io_backender_t *io_backender,
                                               const serializer_filepath_t &filename,
                                               perfmon_collection_t *perfmon_parent,
                                               const auth_semilattice_metadata_t &initial_metadata) :
    persistent_file_t<auth_semilattice_metadata_t>(io_backender, filename, perfmon_parent, true) {
    object_buffer_t<txn_t> txn;
    get_write_transaction(&txn);
    buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                          access_t::write);
    buf_write_t sb_write(&superblock);
    auth_metadata_superblock_t *sb
        = static_cast<auth_metadata_superblock_t *>(sb_write.get_data_write());
    memset(sb, 0, get_cache_block_size().value());
    sb->magic = expected_magic;

    write_blob(buf_parent_t(&superblock),
               sb->metadata_blob,
               auth_metadata_superblock_t::METADATA_BLOB_MAXREFLEN,
               initial_metadata);
}

auth_persistent_file_t::~auth_persistent_file_t() {
    // Do nothing
}

auth_semilattice_metadata_t auth_persistent_file_t::read_metadata() {
    object_buffer_t<txn_t> txn;
    get_read_transaction(&txn);
    buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                          access_t::read);
    buf_read_t sb_read(&superblock);
    const auth_metadata_superblock_t *sb
        = static_cast<const auth_metadata_superblock_t *>(sb_read.get_data_read());
    auth_semilattice_metadata_t metadata;
    read_blob(buf_parent_t(&superblock), sb->metadata_blob,
              auth_metadata_superblock_t::METADATA_BLOB_MAXREFLEN, &metadata);
    return metadata;
}

void auth_persistent_file_t::update_metadata(const auth_semilattice_metadata_t &metadata) {
    // KSI: This and other functions here seem sketchy.  They used order tokens to
    // worry about transaction/superblock acquisition ordering, but they don't
    // actually enforce ordering with some kind of mutex or fifo enforcer.  Or do
    // they?
    object_buffer_t<txn_t> txn;
    get_write_transaction(&txn);
    buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                          access_t::write);
    buf_write_t sb_write(&superblock);

    auth_metadata_superblock_t *sb
        = static_cast<auth_metadata_superblock_t *>(sb_write.get_data_write());
    write_blob(buf_parent_t(&superblock), sb->metadata_blob,
               auth_metadata_superblock_t::METADATA_BLOB_MAXREFLEN, metadata);
}

cluster_persistent_file_t::cluster_persistent_file_t(io_backender_t *io_backender,
                                                     const serializer_filepath_t &filename,
                                                     perfmon_collection_t *perfmon_parent) :
    persistent_file_t<cluster_semilattice_metadata_t>(io_backender, filename, perfmon_parent, false) {
    construct_branch_history_managers(false);
}

cluster_persistent_file_t::cluster_persistent_file_t(io_backender_t *io_backender,
                                                     const serializer_filepath_t &filename,
                                                     perfmon_collection_t *perfmon_parent,
                                                     const machine_id_t &machine_id,
                                                     const cluster_semilattice_metadata_t &initial_metadata) :
    persistent_file_t<cluster_semilattice_metadata_t>(io_backender, filename, perfmon_parent, true) {

    object_buffer_t<txn_t> txn;
    get_write_transaction(&txn);
    buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                          access_t::write);
    buf_write_t sb_write(&superblock);
    cluster_metadata_superblock_t *sb
        = static_cast<cluster_metadata_superblock_t *>(sb_write.get_data_write());

    memset(sb, 0, get_cache_block_size().value());
    sb->magic = expected_magic;
    sb->machine_id = machine_id;
    write_blob(buf_parent_t(&superblock),
               sb->metadata_blob,
               cluster_metadata_superblock_t::METADATA_BLOB_MAXREFLEN,
               initial_metadata);
    write_blob(buf_parent_t(&superblock),
               sb->dummy_branch_history_blob,
               cluster_metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN,
               branch_history_t<mock::dummy_protocol_t>());
    write_blob(buf_parent_t(&superblock),
               sb->memcached_branch_history_blob,
               cluster_metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN,
               branch_history_t<memcached_protocol_t>());
    write_blob(buf_parent_t(&superblock),
               sb->rdb_branch_history_blob,
               cluster_metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN,
               branch_history_t<rdb_protocol_t>());

    construct_branch_history_managers(true);
}

cluster_persistent_file_t::~cluster_persistent_file_t() {
    // Do nothing
}

cluster_semilattice_metadata_t cluster_persistent_file_t::read_metadata() {
    object_buffer_t<txn_t> txn;
    get_read_transaction(&txn);
    buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                          access_t::read);
    buf_read_t sb_read(&superblock);

    const cluster_metadata_superblock_t *sb
        = static_cast<const cluster_metadata_superblock_t *>(sb_read.get_data_read());
    cluster_semilattice_metadata_t metadata;
    read_blob(buf_parent_t(&superblock), sb->metadata_blob,
              cluster_metadata_superblock_t::METADATA_BLOB_MAXREFLEN, &metadata);
    return metadata;
}

void cluster_persistent_file_t::update_metadata(const cluster_semilattice_metadata_t &metadata) {
    object_buffer_t<txn_t> txn;
    get_write_transaction(&txn);
    buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                          access_t::write);
    buf_write_t sb_write(&superblock);
    cluster_metadata_superblock_t *sb
        = static_cast<cluster_metadata_superblock_t *>(sb_write.get_data_write());
    write_blob(buf_parent_t(&superblock), sb->metadata_blob,
               cluster_metadata_superblock_t::METADATA_BLOB_MAXREFLEN, metadata);
}

machine_id_t cluster_persistent_file_t::read_machine_id() {
    object_buffer_t<txn_t> txn;
    get_read_transaction(&txn);
    buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                          access_t::read);
    buf_read_t sb_read(&superblock);
    const cluster_metadata_superblock_t *sb
        = static_cast<const cluster_metadata_superblock_t *>(sb_read.get_data_read());
    return sb->machine_id;
}

template <class protocol_t>
class cluster_persistent_file_t::persistent_branch_history_manager_t : public branch_history_manager_t<protocol_t> {
public:
    persistent_branch_history_manager_t(cluster_persistent_file_t *p,
                                        char (cluster_metadata_superblock_t::*fn)[cluster_metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN],
                                        bool create) :
        parent(p), field_name(fn)
    {
        /* If we're not creating, we have to load the existing branch history
        database from disk */
        if (!create) {
            object_buffer_t<txn_t> txn;
            parent->get_read_transaction(&txn);
            buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                                  access_t::read);
            buf_read_t sb_read(&superblock);
            const cluster_metadata_superblock_t *sb
                = static_cast<const cluster_metadata_superblock_t *>(sb_read.get_data_read());
            read_blob(buf_parent_t(&superblock), sb->*field_name,
                      cluster_metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN,
                      &bh);
        }
    }

    branch_birth_certificate_t<protocol_t> get_branch(branch_id_t branch) THROWS_NOTHING {
        home_thread_mixin_t::assert_thread();
        typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::const_iterator it = bh.branches.find(branch);
        guarantee(it != bh.branches.end(), "no such branch");
        return it->second;
    }

    std::set<branch_id_t> known_branches() THROWS_NOTHING {
        std::set<branch_id_t> res;

        for (typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::iterator it  = bh.branches.begin();
                                                                                               it != bh.branches.end();
                                                                                               ++it) {
            res.insert(it->first);
        }

        return res;
    }

    void create_branch(branch_id_t branch_id,
                       const branch_birth_certificate_t<protocol_t> &bc,
                       signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        std::pair<typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::iterator, bool>
            insert_res = bh.branches.insert(std::make_pair(branch_id, bc));
        guarantee(insert_res.second);
        flush(interruptor);
    }

    void export_branch_history(branch_id_t branch,
                               branch_history_t<protocol_t> *out) THROWS_NOTHING {
        home_thread_mixin_t::assert_thread();
        std::set<branch_id_t> to_process;
        if (out->branches.count(branch) == 0) {
            to_process.insert(branch);
        }
        while (!to_process.empty()) {
            branch_id_t next = *to_process.begin();
            to_process.erase(next);
            branch_birth_certificate_t<protocol_t> bc = get_branch(next);
            std::pair<typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::iterator, bool>
                insert_res = out->branches.insert(std::make_pair(next, bc));
            guarantee(insert_res.second);
            for (typename region_map_t<protocol_t, version_range_t>::const_iterator it = bc.origin.begin(); it != bc.origin.end(); it++) {
                if (!it->second.latest.branch.is_nil() && out->branches.count(it->second.latest.branch) == 0) {
                    to_process.insert(it->second.latest.branch);
                }
            }
        }
    }

    void import_branch_history(const branch_history_t<protocol_t> &new_records,
                               signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        for (typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::const_iterator it = new_records.branches.begin(); it != new_records.branches.end(); it++) {
            bh.branches.insert(std::make_pair(it->first, it->second));
        }
        flush(interruptor);
    }

private:
    void flush(UNUSED signal_t *interruptor) {
        object_buffer_t<txn_t> txn;
        parent->get_write_transaction(&txn);
        buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                              access_t::write);
        buf_write_t sb_write(&superblock);
        cluster_metadata_superblock_t *sb
            = static_cast<cluster_metadata_superblock_t *>(sb_write.get_data_write());
        write_blob(buf_parent_t(&superblock), sb->*field_name,
                   cluster_metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN, bh);
    }

    cluster_persistent_file_t *parent;
    char (cluster_metadata_superblock_t::*field_name)[cluster_metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN];
    branch_history_t<protocol_t> bh;
};

/* These must be defined when the definition of
`persistent_branch_history_manager_t` is in scope, so that the compiler knows
that `persistent_branch_history_manager_t *` can be implicitly cast to
`branch_history_manager_t *`. */

branch_history_manager_t<mock::dummy_protocol_t> *cluster_persistent_file_t::get_dummy_branch_history_manager() {
    return dummy_branch_history_manager.get();
}

branch_history_manager_t<memcached_protocol_t> *cluster_persistent_file_t::get_memcached_branch_history_manager() {
    return memcached_branch_history_manager.get();
}

branch_history_manager_t<rdb_protocol_t> *cluster_persistent_file_t::get_rdb_branch_history_manager() {
    return rdb_branch_history_manager.get();
}

void cluster_persistent_file_t::construct_branch_history_managers(bool create) {
    dummy_branch_history_manager.init(new persistent_branch_history_manager_t<mock::dummy_protocol_t>(
        this, &cluster_metadata_superblock_t::dummy_branch_history_blob, create));
    memcached_branch_history_manager.init(new persistent_branch_history_manager_t<memcached_protocol_t>(
        this, &cluster_metadata_superblock_t::memcached_branch_history_blob, create));
    rdb_branch_history_manager.init(new persistent_branch_history_manager_t<rdb_protocol_t>(
        this, &cluster_metadata_superblock_t::rdb_branch_history_blob, create));
}

template <class metadata_t>
semilattice_watching_persister_t<metadata_t>::semilattice_watching_persister_t(
        persistent_file_t<metadata_t> *persistent_file_,
        boost::shared_ptr<semilattice_read_view_t<metadata_t> > v) :
    persistent_file(persistent_file_), view(v),
    flush_again(new cond_t),
    subs(boost::bind(&semilattice_watching_persister_t::on_change, this), v)
{
    coro_t::spawn_sometime(boost::bind(&semilattice_watching_persister_t::dump_loop, this, auto_drainer_t::lock_t(&drainer)));
}

template <class metadata_t>
void semilattice_watching_persister_t<metadata_t>::dump_loop(auto_drainer_t::lock_t keepalive) {
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
    } catch (const interrupted_exc_t &) {
        // do nothing
    }
    stopped.pulse();
}

template <class metadata_t>
void semilattice_watching_persister_t<metadata_t>::on_change() {
    if (!flush_again->is_pulsed()) {
        flush_again->pulse();
    }
}

template class semilattice_watching_persister_t<cluster_semilattice_metadata_t>;
template class semilattice_watching_persister_t<auth_semilattice_metadata_t>;

}  // namespace metadata_persistence
