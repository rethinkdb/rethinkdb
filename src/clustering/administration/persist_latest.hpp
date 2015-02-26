// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_LATEST_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_LATEST_HPP_

namespace persist_latest {

class file_in_use_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "metadata file is being used by another rethinkdb process";
    }
};

class metadata_file_t {
public:
    metadata_file_t(
        io_backender_t *io_backender,
        const serializer_filepath_t &filename,
        perfmon_collection_t *perfmon_parent,
        bool create);

private:
    scoped_ptr_t<standard_serializer_t> serializer;
    scoped_ptr_t<cache_balancer_t> balancer;
    scoped_ptr_t<cache_t> cache;
    scoped_ptr_t<cache_conn_t> cache_conn;
};

class metadata_read_txn_t {
public:
    metadata_read_txn_t(metadata_file_t *file, signal_t *interruptor);

    template<class T>
    bool read(
            const std::string &key,
            T *value_out,
            signal_t *interruptor) {
        bool found = false;
        read_bin(
            key,
            [&](read_stream_t *bin_value) {
                archive_result_t res = deserialize<cluster_version_t::LATEST_DISK>(
                    bin_value, value_out);
                guarantee_deserialization(res,
                    "persistent_file_t::read key='%s'", key.c_str());
                found = true;
            },
            interruptor);
        return found;
    }

    template<class T>
    void read_many(
            const std::string &key_prefix,
            const std::function<void(const std::string &key_suffix, const T &value)> &cb,
            signal_t *interruptor) {
        read_many_bin(
            key_prefix,
            [&](const std::string &key_suffix, read_stream_t *bin_value) {
                T value;
                archive_result_t res = deserialize<cluster_version_t::LATEST_DISK>(
                    bin_value, &value);
                guarantee_deserialization(res,
                    "persistent_file_t::read_many key_prefix='%s'", key_prefix.c_str());
                cb(key_suffix, value);
            },
            interruptor);
    }

private:
    void read_bin(
        const std::string &key,
        const std::function<void(read_stream_t *)> &callback,
        signal_t *interruptor);

    void read_many_bin(
        const std::string &key_prefix,
        const std::function<void(const std::string &key_suffix, read_stream_t *)> &cb,
        signal_t *interruptor);
};

class metadata_write_txn_t : public metadata_read_txn_t {
public:
    metadata_write_txn_t(metadata_file_t *file, signal_t *interruptor);

    template<class T>
    void write(
            const std::string &key,
            const T &value,
            signal_t *interruptor) {
        write_message_t wm;
        serialize<cluster_version_t::LATEST_DISK>(&wm, value);
        write_bin(key, wm, interruptor);
    }

    void erase(const std::string &key, signal_t *interruptor);

    void commit(signal_t *interruptor);

private:
    void write_bin(
        const std::string &key,
        const write_message_t &msg,
        signal_t *interruptor);
};

class real_table_meta_persistence_interface_t :
    public table_meta_persistence_interface_t {
public:
    void read_all_tables(
            const std::function<void(
                const namespace_id_t &table_id,
                const table_meta_persistent_state_t &state,
                scoped_ptr_t<multistore_ptr_t> &&multistore_ptr)> &callback,
            signal_t *interruptor);
    void add_table(
            const namespace_id_t &table,
            const table_meta_persistent_state_t &state,
            scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
            signal_t *interruptor);
    void update_table(
            const namespace_id_t &table,
            const table_meta_persistent_state_t &state,
            signal_t *interruptor);
    void remove_table(
            const namespace_id_t &table,
            signal_t *interruptor);
};

} /* namespace persist_latest */

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_LATEST_HPP_ */

