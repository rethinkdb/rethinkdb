#include "clustering/administration/persist.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include "arch/runtime/thread_pool.hpp"
#include "containers/archive/string_stream.hpp"

namespace metadata_persistence {

std::string metadata_file(const std::string& file_path) {
    return file_path + "/metadata";
}

std::string errno_to_string(int err) {
    char buffer[200];
    char *res = strerror_r(err, buffer, sizeof(buffer));
    return std::string(res);
}

bool check_existence(const std::string& file_path) THROWS_ONLY(file_exc_t) {
    int res = access(file_path.c_str(), F_OK);
    if (res == 0) {
        return true;
    } else if (errno == ENOENT) {
        return false;
    } else {
        throw file_exc_t(errno_to_string(errno));
    }
}

void create(const std::string& file_path, const machine_id_t &machine_id, const cluster_semilattice_metadata_t &semilattice) {
    persistent_file_t store(metadata_file(file_path), true);
    store.update(machine_id, semilattice, true);
}

void read(const std::string& file_path, machine_id_t *machine_id_out, cluster_semilattice_metadata_t *semilattice_out) {    
    persistent_file_t store(metadata_file(file_path));
    store.read(machine_id_out, semilattice_out);
}

semilattice_watching_persister_t::semilattice_watching_persister_t(
        const std::string &filepath,
        machine_id_t mi,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > v) :
    persistent_file(metadata_file(filepath)), machine_id(mi), view(v),
    flush_again(new cond_t),
    subs(boost::bind(&semilattice_watching_persister_t::on_change, this), v)
{
    coro_t::spawn_sometime(boost::bind(&semilattice_watching_persister_t::dump_loop, this, auto_drainer_t::lock_t(&drainer)));
}

void semilattice_watching_persister_t::dump_loop(auto_drainer_t::lock_t keepalive) {
    try {
        for (;;) {
            persistent_file.update(machine_id, view->get());
            {
                wait_any_t c(flush_again.get(), &stop);
                wait_interruptible(&c, keepalive.get_drain_signal());
            }
            if (flush_again->is_pulsed()) {
                flush_again.reset(new cond_t);
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

const block_magic_t blob_superblock_t::expected_magic = { { 'b', 'l', 'o', 'b' } };

persistent_file_t::persistent_file_t(const std::string& filename, bool create) {
    if (create) {
        standard_serializer_t::create(
            standard_serializer_t::dynamic_config_t(),
            standard_serializer_t::private_dynamic_config_t(filename),
            standard_serializer_t::static_config_t()
        );
    }

    serializer.reset(new standard_serializer_t(
        standard_serializer_t::dynamic_config_t(),
        standard_serializer_t::private_dynamic_config_t(filename),
        NULL)
    );

    if (create) {
        mirrored_cache_static_config_t cache_static_config;
        cache_t::create(serializer.get(), &cache_static_config);
    }

    cache_dynamic_config.flush_timer_ms = 1000;
    cache_dynamic_config.max_size = MEGABYTE;
    cache_dynamic_config.max_dirty_size = MEGABYTE / 2;
    cache.reset(new cache_t(serializer.get(), &cache_dynamic_config, NULL));
}

void persistent_file_t::update(const machine_id_t &machine_id, const cluster_semilattice_metadata_t &semilattice, bool create) {
    transaction_t txn(cache.get(), rwi_write, 1, repli_timestamp_t::distant_past);

    buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_write);

    blob_superblock_t *sb = static_cast<blob_superblock_t *>(superblock.get_data_major_write());

    if (create) {
        // we need to initialize the superblock
        bzero(sb, cache->get_block_size().value());
        sb->magic = blob_superblock_t::expected_magic;
    }

    blob_t blob(sb->metainfo_blob, blob_superblock_t::METADATA_BLOB_MAXREFLEN);

    write_message_t msg;

    semilattice.rdb_serialize(msg);

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

    rassert(str.size() == slen);

    int id_len = sizeof(machine_id_t);

    blob.clear(&txn);
    blob.append_region(&txn, str.size() + id_len);
    blob.write_from_string(std::string(reinterpret_cast<const char*>(&machine_id), id_len), &txn, 0);
    blob.write_from_string(str, &txn, id_len);

    rassert(blob.valuesize() == static_cast<int64_t>(slen + id_len));
}

void persistent_file_t::read(machine_id_t *machine_id_out, cluster_semilattice_metadata_t *semilattice_out) {
    transaction_t txn(cache.get(), rwi_read, 0, repli_timestamp_t::distant_past);

    buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_read);

    blob_superblock_t *sb = const_cast<blob_superblock_t *>(static_cast<const blob_superblock_t *>(superblock.get_data_read()));

    blob_t blob(sb->metainfo_blob, blob_superblock_t::METADATA_BLOB_MAXREFLEN);

    std::string str;

    blob.read_to_string(str, &txn, 0, blob.valuesize());

    read_string_stream_t ss(str);

    guarantee(ss.read(static_cast<void *>(machine_id_out), sizeof(machine_id_t)) != 0);

    semilattice_out->rdb_deserialize(&ss);
}

}  // namespace metadata_persistence
