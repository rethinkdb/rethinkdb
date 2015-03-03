// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/persist.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <functional>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/thread_pool.hpp"
#include "buffer_cache/alt.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "concurrency/throttled_committer.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "clustering/immediate_consistency/history.hpp"
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

    server_id_t server_id;

    static const int METADATA_BLOB_MAXREFLEN = 1500;
    char metadata_blob[METADATA_BLOB_MAXREFLEN];

    static const int BRANCH_HISTORY_BLOB_MAXREFLEN = 500;
    char rdb_branch_history_blob[BRANCH_HISTORY_BLOB_MAXREFLEN];
};

// Etymology: (R)ethink(D)B (m)eta(d)ata
// Yes, v1_13 magic is the same for both cluster and auth metadata.
template <cluster_version_t>
struct cluster_metadata_magic_t {
    static const block_magic_t value;
};

template <>
const block_magic_t
    cluster_metadata_magic_t<cluster_version_t::v1_13>::value
    = { { 'R', 'D', 'm', 'd' } };
template <>
const block_magic_t
    cluster_metadata_magic_t<cluster_version_t::v1_14>::value
    = { { 'R', 'D', 'm', 'e' } };
template <>
const block_magic_t
    cluster_metadata_magic_t<cluster_version_t::v1_15>::value
    = { { 'R', 'D', 'm', 'f' } };
template <>
const block_magic_t
    cluster_metadata_magic_t<cluster_version_t::v1_16_is_latest_disk>::value
    = { { 'R', 'D', 'm', 'g' } };

template <cluster_version_t>
struct auth_metadata_magic_t {
    static const block_magic_t value;
};

template <>
const block_magic_t auth_metadata_magic_t<cluster_version_t::v1_13>::value
    = { { 'R', 'D', 'm', 'd' } };
template <>
const block_magic_t auth_metadata_magic_t<cluster_version_t::v1_14>::value
    = { { 'R', 'D', 'm', 'e' } };
template <>
const block_magic_t auth_metadata_magic_t<cluster_version_t::v1_15>::value
    = { { 'R', 'D', 'm', 'f' } };
template <>
const block_magic_t auth_metadata_magic_t<cluster_version_t::v1_16_is_latest_disk>::value
    = { { 'R', 'D', 'm', 'g' } };

cluster_version_t auth_superblock_version(const auth_metadata_superblock_t *sb) {
    if (sb->magic
        == auth_metadata_magic_t<cluster_version_t::v1_13>::value) {
        return cluster_version_t::v1_13;
    } else if (sb->magic
               == auth_metadata_magic_t<cluster_version_t::v1_14>::value) {
        return cluster_version_t::v1_14;
    } else if (sb->magic
               == auth_metadata_magic_t<cluster_version_t::v1_15>::value) {
        return cluster_version_t::v1_15;
    } else if (sb->magic
               == auth_metadata_magic_t<cluster_version_t::v1_16_is_latest_disk>::value) {
        return cluster_version_t::v1_16_is_latest_disk;
    } else {
        crash("auth_metadata_superblock_t has invalid magic.");
    }
}

// This only takes a cluster_version_t parameter to get people thinking -- you have
// to use LATEST_DISK.
template <cluster_version_t W, class T>
static void write_blob(buf_parent_t parent, char *ref, int maxreflen,
                       const T &value) {
    static_assert(W == cluster_version_t::LATEST_DISK,
                  "You must serialize with the latest version.  Make sure to update "
                  "the rest of the blob, too!");
    write_message_t wm;
    serialize<W>(&wm, value);
    intrusive_list_t<write_buffer_t> *buffers = wm.unsafe_expose_buffers();
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
    blob_t blob(parent.cache()->max_block_size(), ref, maxreflen);
    blob.clear(parent);
    blob.append_region(parent, str.size());
    blob.write_from_string(str, parent, 0);
    guarantee(blob.valuesize() == static_cast<int64_t>(slen));
}

static void read_blob(buf_parent_t parent, const char *ref, int maxreflen,
                      const std::function<archive_result_t(read_stream_t *)> &reader) {
    blob_t blob(parent.cache()->max_block_size(),
                const_cast<char *>(ref), maxreflen);
    blob_acq_t acq_group;
    buffer_group_t group;
    blob.expose_all(parent, access_t::read, &group, &acq_group);
    buffer_group_read_stream_t ss(const_view(&group));
    archive_result_t res = reader(&ss);
    guarantee_deserialization(res, "T (template code)");
}


cluster_version_t cluster_superblock_version(const cluster_metadata_superblock_t *sb) {
    if (sb->magic
        == cluster_metadata_magic_t<cluster_version_t::v1_13>::value) {
        return cluster_version_t::v1_13;
    } else if (sb->magic
               == cluster_metadata_magic_t<cluster_version_t::v1_14>::value) {
        return cluster_version_t::v1_14;
    } else if (sb->magic
               == cluster_metadata_magic_t<cluster_version_t::v1_15>::value) {
        return cluster_version_t::v1_15;
    } else if (sb->magic
               == cluster_metadata_magic_t<cluster_version_t::v1_16_is_latest_disk>::value) {
        return cluster_version_t::v1_16_is_latest_disk;
    } else {
        crash("cluster_metadata_superblock_t has invalid magic.");
    }
}

void read_metadata_blob(buf_parent_t sb_buf,
                        const cluster_metadata_superblock_t *sb,
                        cluster_semilattice_metadata_t *out) {
    cluster_version_t v = cluster_superblock_version(sb);
    if (v == cluster_version_t::v1_13 || v == cluster_version_t::v1_13_2 ||
            v == cluster_version_t::v1_14 || v == cluster_version_t::v1_15) {
        crash("Metadata is too old. We can't migrate from versions earlier than 1.16.");
    } else {
        read_blob(
            sb_buf,
            sb->metadata_blob,
            cluster_metadata_superblock_t::METADATA_BLOB_MAXREFLEN,
            [&](read_stream_t *s) -> archive_result_t {
                switch (v) {
                    case cluster_version_t::v1_16_is_latest:
                        return deserialize<cluster_version_t::v1_16_is_latest>(s, out);
                    case cluster_version_t::v1_13:
                    case cluster_version_t::v1_13_2:
                    case cluster_version_t::v1_14:
                    case cluster_version_t::v1_15:
                    default:
                        unreachable();
                }
            });
    }
}

// As with write_blob, the template parameter must be cluster_version_t::LATEST_DISK.
// Use bring_up_to_date before calling this.
template <cluster_version_t W>
void write_metadata_blob(buf_parent_t sb_buf,
                         cluster_metadata_superblock_t *sb,
                         const cluster_semilattice_metadata_t &metadata) {
    guarantee(cluster_superblock_version(sb) == W);
    write_blob<W>(sb_buf,
                  sb->metadata_blob,
                  cluster_metadata_superblock_t::METADATA_BLOB_MAXREFLEN,
                  metadata);
}

void read_branch_history_blob(buf_parent_t sb_buf,
                              const cluster_metadata_superblock_t *sb,
                              branch_history_t *out) {
    cluster_version_t v = cluster_superblock_version(sb);
    /* RSI(raft): Support branch history migration */
    guarantee(v == cluster_version_t::LATEST_OVERALL);
    read_blob(
        sb_buf,
        sb->rdb_branch_history_blob,
        cluster_metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN,
        [&](read_stream_t *s) -> archive_result_t {
            return deserialize<cluster_version_t::LATEST_OVERALL>(s, out);
        });
}

// As with write_blob, the template parameter must be cluster_version_t::LATEST_DISK.
// Use bring_up_to_date before calling this.
template <cluster_version_t W>
void write_branch_history_blob(buf_parent_t sb_buf,
                               cluster_metadata_superblock_t *sb,
                               const branch_history_t &history) {
    guarantee(cluster_superblock_version(sb) == W);
    write_blob<W>(sb_buf,
                  sb->rdb_branch_history_blob,
                  cluster_metadata_superblock_t::BRANCH_HISTORY_BLOB_MAXREFLEN,
                  history);
}

void bring_up_to_date(
        buf_parent_t sb_buf,
        cluster_metadata_superblock_t *sb) {
    if (cluster_superblock_version(sb) == cluster_version_t::LATEST_DISK) {
        // Usually we're already up to date.
        return;
    }

    cluster_semilattice_metadata_t metadata;
    read_metadata_blob(sb_buf, sb, &metadata);

    branch_history_t branch_history;
    read_branch_history_blob(sb_buf, sb, &branch_history);

    // Now that we've read everything, do writes of anything whose serialization may
    // have changed between versions.

    // (We must write the magic _first_ to appease the assertions in
    // write_metadata_blob and write_branch_history_blob that check we've called
    // bring_up_to_date.)
    sb->magic = cluster_metadata_magic_t<cluster_version_t::LATEST_DISK>::value;
    write_metadata_blob<cluster_version_t::LATEST_DISK>(sb_buf, sb, metadata);
    write_branch_history_blob<cluster_version_t::LATEST_DISK>(sb_buf, sb, branch_history);
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
    return cache->max_block_size();
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
        persistent_file_t<auth_semilattice_metadata_t>(io_backender, filename,
                                                       perfmon_parent, false) {
    /* Force migration to happen */
    update_metadata(read_metadata());
}

auth_persistent_file_t::auth_persistent_file_t(io_backender_t *io_backender,
                                               const serializer_filepath_t &filename,
                                               perfmon_collection_t *perfmon_parent,
                                               const auth_semilattice_metadata_t &initial_metadata) :
        persistent_file_t<auth_semilattice_metadata_t>(io_backender, filename,
                                                       perfmon_parent, true) {
    object_buffer_t<txn_t> txn;
    get_write_transaction(&txn);
    buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                          access_t::write);
    buf_write_t sb_write(&superblock);
    auth_metadata_superblock_t *sb
        = static_cast<auth_metadata_superblock_t *>(sb_write.get_data_write());
    memset(sb, 0, get_cache_block_size().value());
    sb->magic = auth_metadata_magic_t<cluster_version_t::LATEST_DISK>::value;

    write_blob<cluster_version_t::LATEST_DISK>(
            buf_parent_t(&superblock),
            sb->metadata_blob,
            auth_metadata_superblock_t::METADATA_BLOB_MAXREFLEN,
            initial_metadata);

    rassert(auth_superblock_version(sb) == cluster_version_t::LATEST_DISK);
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
    cluster_version_t v = auth_superblock_version(sb);
    if (v == cluster_version_t::v1_13 || v == cluster_version_t::v1_13_2 ||
            v == cluster_version_t::v1_14 || v == cluster_version_t::v1_15) {
        crash("Metadata is too old. We can't migrate from versions earlier than 1.16.");
    } else {
        read_blob(
            buf_parent_t(&superblock),
            sb->metadata_blob,
            auth_metadata_superblock_t::METADATA_BLOB_MAXREFLEN,
            [&](read_stream_t *s) -> archive_result_t {
                switch (v) {
                    case cluster_version_t::v1_16_is_latest:
                        return deserialize<cluster_version_t::v1_16_is_latest>(
                            s, &metadata);
                    case cluster_version_t::v1_13:
                    case cluster_version_t::v1_13_2:
                    case cluster_version_t::v1_14:
                    case cluster_version_t::v1_15:
                    default:
                        unreachable();
                }
            });
    }
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
    // There is just one metadata_blob in auth_metadata_superblock_t (for now), so it
    // suffices to serialize with the latest version.
    sb->magic = auth_metadata_magic_t<cluster_version_t::LATEST_DISK>::value;

    write_blob<cluster_version_t::LATEST_DISK>(
            buf_parent_t(&superblock), sb->metadata_blob,
            auth_metadata_superblock_t::METADATA_BLOB_MAXREFLEN, metadata);
}

cluster_persistent_file_t::cluster_persistent_file_t(io_backender_t *io_backender,
                                                     const serializer_filepath_t &filename,
                                                     perfmon_collection_t *perfmon_parent) :
        persistent_file_t<cluster_semilattice_metadata_t>(io_backender, filename,
                                                          perfmon_parent, false) {
    construct_branch_history_managers(false);

    /* Force migration to happen */
    update_metadata(read_metadata());
}

cluster_persistent_file_t::cluster_persistent_file_t(io_backender_t *io_backender,
                                                     const serializer_filepath_t &filename,
                                                     perfmon_collection_t *perfmon_parent,
                                                     const server_id_t &server_id,
                                                     const cluster_semilattice_metadata_t &initial_metadata) :
        persistent_file_t<cluster_semilattice_metadata_t>(io_backender, filename,
                                                          perfmon_parent, true) {
    object_buffer_t<txn_t> txn;
    get_write_transaction(&txn);
    buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                          access_t::write);
    buf_write_t sb_write(&superblock);
    cluster_metadata_superblock_t *sb
        = static_cast<cluster_metadata_superblock_t *>(sb_write.get_data_write());

    memset(sb, 0, get_cache_block_size().value());
    sb->magic = cluster_metadata_magic_t<cluster_version_t::LATEST_DISK>::value;
    sb->server_id = server_id;
    write_metadata_blob<cluster_version_t::LATEST_DISK>(
            buf_parent_t(&superblock), sb, initial_metadata);
    write_branch_history_blob<cluster_version_t::LATEST_DISK>(
            buf_parent_t(&superblock),
            sb,
            branch_history_t());

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
    read_metadata_blob(buf_parent_t(&superblock), sb, &metadata);
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
    bring_up_to_date(buf_parent_t(&superblock), sb);
    write_metadata_blob<cluster_version_t::LATEST_DISK>(
            buf_parent_t(&superblock), sb, metadata);
}

server_id_t cluster_persistent_file_t::read_server_id() {
    object_buffer_t<txn_t> txn;
    get_read_transaction(&txn);
    buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                          access_t::read);
    buf_read_t sb_read(&superblock);
    const cluster_metadata_superblock_t *sb
        = static_cast<const cluster_metadata_superblock_t *>(sb_read.get_data_read());
    return sb->server_id;
}

class cluster_persistent_file_t::persistent_branch_history_manager_t : public branch_history_manager_t {
public:
    persistent_branch_history_manager_t(cluster_persistent_file_t *p,
                                        bool create)
        : parent(p),
          flusher(std::bind(&persistent_branch_history_manager_t::flush, this), 1) {
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
            read_branch_history_blob(buf_parent_t(&superblock),
                                     sb,
                                     &bh);
        }
    }

    branch_birth_certificate_t get_branch(const branch_id_t &branch)
            const THROWS_NOTHING {
        home_thread_mixin_t::assert_thread();
        return bh.get_branch(branch);
    }

    bool is_branch_known(const branch_id_t &branch) const THROWS_NOTHING {
        home_thread_mixin_t::assert_thread();
        return bh.is_branch_known(branch);
    }

    void create_branch(branch_id_t branch_id,
                       const branch_birth_certificate_t &bc,
                       UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        std::pair<std::map<branch_id_t, branch_birth_certificate_t>::iterator, bool>
            insert_res = bh.branches.insert(std::make_pair(branch_id, bc));
        guarantee(insert_res.second);
        flusher.sync();
    }

    void import_branch_history(const branch_history_t &new_records,
                               UNUSED signal_t *interruptor)
                               THROWS_ONLY(interrupted_exc_t) {
        home_thread_mixin_t::assert_thread();
        for (std::map<branch_id_t, branch_birth_certificate_t>::const_iterator it = new_records.branches.begin(); it != new_records.branches.end(); it++) {
            bh.branches.insert(std::make_pair(it->first, it->second));
        }
        flusher.sync();
    }

private:
    void flush() {
        object_buffer_t<txn_t> txn;
        parent->get_write_transaction(&txn);
        buf_lock_t superblock(buf_parent_t(txn.get()), SUPERBLOCK_ID,
                              access_t::write);
        buf_write_t sb_write(&superblock);
        cluster_metadata_superblock_t *sb
            = static_cast<cluster_metadata_superblock_t *>(sb_write.get_data_write());
        bring_up_to_date(buf_parent_t(&superblock), sb);
        write_branch_history_blob<cluster_version_t::LATEST_DISK>(
                buf_parent_t(&superblock), sb, bh);
    }

    cluster_persistent_file_t *parent;
    branch_history_t bh;
    throttled_committer_t flusher;
};

/* These must be defined when the definition of
`persistent_branch_history_manager_t` is in scope, so that the compiler knows
that `persistent_branch_history_manager_t *` can be implicitly cast to
`branch_history_manager_t *`. */

branch_history_manager_t *cluster_persistent_file_t::get_rdb_branch_history_manager() {
    return rdb_branch_history_manager.get();
}

void cluster_persistent_file_t::construct_branch_history_managers(bool create) {
    rdb_branch_history_manager.init(new persistent_branch_history_manager_t(
        this, create));
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
