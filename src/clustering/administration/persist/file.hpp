// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_FILE_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_FILE_HPP_

#include "btree/operations.hpp"
#include "buffer_cache/alt.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/rwlock.hpp"
#include "serializer/types.hpp"

class io_backender_t;
class merger_serializer_t;
class filepath_file_opener_t;

class file_in_use_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "metadata file is being used by another rethinkdb process";
    }
};

class metadata_file_t {
public:
    template<class T>
    class key_t {
    public:
        explicit key_t(const std::string &s) : key(s) { }
        key_t suffix(const std::string &s) const {
            key_t copy = *this;
            guarantee(key.size() + s.size() <= MAX_KEY_SIZE);
            copy.key.set_size(key.size() + s.size());
            memcpy(copy.key.contents() + key.size(), s.c_str(), s.size());
            return copy;
        }
    private:
        friend class metadata_file_t;
        store_key_t key;
    };

    class read_txn_t {
    public:
        read_txn_t(metadata_file_t *file, signal_t *interruptor);

        template<class T>
        T read(const key_t<T> &key, signal_t *interruptor) {
            T value;
            bool found = read_maybe(key, &value, interruptor);
            guarantee(found, "failed to find expected metadata key");
            return value;
        }

        template<class T>
        bool read_maybe(
                const key_t<T> &key,
                T *value_out,
                signal_t *interruptor) {
            bool found = false;
            read_bin(
                key.key,
                [&](read_stream_t *bin_value) {
                    archive_result_t res =
                        deserialize<cluster_version_t::v2_1_is_latest>(
                            bin_value, value_out);
                    guarantee_deserialization(res, "metadata_file_t::read_txn_t::read");
                    found = true;
                },
                interruptor);
            return found;
        }

        template<class T>
        void read_many(
                const key_t<T> &key_prefix,
                const std::function<void(
                    const std::string &key_suffix, const T &value)> &cb,
                signal_t *interruptor) {
            read_many_bin(
                key_prefix.key,
                [&](const std::string &key_suffix, read_stream_t *bin_value) {
                    T value;
                    archive_result_t res =
                        deserialize<cluster_version_t::v2_1_is_latest>(
                            bin_value, &value);
                    guarantee_deserialization(res,
                        "metadata_file_t::read_txn_t::read_many");
                    cb(key_suffix, value);
                },
                interruptor);
        }

    private:
        friend class metadata_file_t;

        /* This constructor is used by `write_txn_t` */
        read_txn_t(metadata_file_t *file, write_access_t, signal_t *interruptor);

        void blob_to_stream(
            buf_parent_t parent,
            const void *ref,
            const std::function<void(read_stream_t *)> &callback);

        void read_bin(
            const store_key_t &key,
            const std::function<void(read_stream_t *)> &callback,
            signal_t *interruptor);

        void read_many_bin(
            const store_key_t &key_prefix,
            const std::function<void(
                const std::string &key_suffix, read_stream_t *)> &cb,
            signal_t *interruptor);

        metadata_file_t *file;
        txn_t txn;
        rwlock_acq_t rwlock_acq;
    };

    class write_txn_t : public read_txn_t {
    public:
        write_txn_t(metadata_file_t *file, signal_t *interruptor);

        template<class T>
        void write(
                const key_t<T> &key,
                const T &value,
                signal_t *interruptor) {
            write_message_t wm;
            serialize<cluster_version_t::LATEST_DISK>(&wm, value);
            write_bin(key.key, &wm, interruptor);
        }

        template<class T>
        void erase(const key_t<T> &key, signal_t *interruptor) {
            write_bin(key.key, nullptr, interruptor);
        }

    private:
        friend class metadata_file_t;

        void write_bin(
            const store_key_t &key,
            const write_message_t *msg,
            signal_t *interruptor);
    };

    // Used to open an existing metadata file
    metadata_file_t(
        io_backender_t *io_backender,
        const base_path_t &base_path,
        bool migrate_inconsistent_data,
        perfmon_collection_t *perfmon_parent,
        signal_t *interruptor);

    // Used top create a new metadata file
    metadata_file_t(
        io_backender_t *io_backender,
        const base_path_t &base_path,
        perfmon_collection_t *perfmon_parent,
        const std::function<void(write_txn_t *, signal_t *)> &initializer,
        signal_t *interruptor);
    ~metadata_file_t();

private:
    void init_serializer(
        filepath_file_opener_t *file_opener,
        perfmon_collection_t *perfmon_parent);

    static serializer_filepath_t get_filename(const base_path_t &path);

    scoped_ptr_t<merger_serializer_t> serializer;
    scoped_ptr_t<cache_balancer_t> balancer;
    scoped_ptr_t<cache_t> cache;
    scoped_ptr_t<cache_conn_t> cache_conn;
    btree_stats_t btree_stats;
    rwlock_t rwlock;
};

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_FILE_HPP_ */

