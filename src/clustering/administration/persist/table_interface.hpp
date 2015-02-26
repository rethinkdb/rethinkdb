// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_TABLE_INTERFACE_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_TABLE_INTERFACE_HPP_

class dummy_table_meta_persistence_interface_t :
    public table_meta_persistence_interface_t {
public:
    void read_all_tables(
            UNUSED const std::function<void(
                const namespace_id_t &table_id,
                const table_meta_persistent_state_t &state,
                scoped_ptr_t<multistore_ptr_t> &&multistore_ptr)> &callback,
            UNUSED signal_t *interruptor) {
    }
    void add_table(
            UNUSED const namespace_id_t &table,
            UNUSED const table_meta_persistent_state_t &state,
            UNUSED scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
            UNUSED signal_t *interruptor) {
    }
    void update_table(
            UNUSED const namespace_id_t &table,
            UNUSED const table_meta_persistent_state_t &state,
            UNUSED signal_t *interruptor) {
    }
    void remove_table(
            UNUSED const namespace_id_t &table,
            UNUSED signal_t *interruptor) {
    }
};

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_TABLE_INTERFACE_HPP_ */
