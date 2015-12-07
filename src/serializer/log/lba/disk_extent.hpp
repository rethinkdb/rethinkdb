// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_LBA_DISK_EXTENT_HPP_
#define SERIALIZER_LOG_LBA_DISK_EXTENT_HPP_

#include "containers/intrusive_list.hpp"
#include "arch/types.hpp"
#include "serializer/log/lba/extent.hpp"
#include "serializer/log/lba/disk_format.hpp"
#include "serializer/log/lba/in_memory_index.hpp"

class extent_manager_t;
class extent_transaction_t;

class lba_disk_extent_t :
    public intrusive_list_node_t<lba_disk_extent_t> {
private:
    extent_manager_t *em;

public:
    extent_t *data;
    int count;

    lba_disk_extent_t(extent_manager_t *_em, file_t *file, file_account_t *io_account);

    lba_disk_extent_t(extent_manager_t *_em, file_t *file, int64_t _offset, int _count);

    bool full() {
        return data->amount_filled == em->extent_size;
    }

    void add_entry(lba_entry_t entry, file_account_t *io_account);

    void sync(file_account_t *io_account, extent_t::sync_callback_t *cb);

    /* To read from an LBA on disk, first call read_step_1(), passing it the address of a
    new read_info_t structure. When it calls the callback you provide, then call
    read_step_2() with the same read_info_t as before and with a pointer to the
    in_memory_index_t to be filled with data. */

    struct read_info_t {
        scoped_device_block_aligned_ptr_t<lba_extent_t> buffer;
        int count;
    };

    void read_step_1(read_info_t *info_out, extent_t::read_callback_t *cb);
    void read_step_2(read_info_t *info, in_memory_index_t *index);

    /* destroy() deletes the structure in memory and also tells the extent manager that the extent
    can be safely reused */
    void destroy(extent_transaction_t *txn) {
        data->destroy(txn);
        delete this;
    }

    /* shutdown() only deletes the structure in memory */
    void shutdown() {
        data->shutdown();
        delete this;
    }

private:
    /* Use destroy() or shutdown() instead */
    ~lba_disk_extent_t() {}

    DISABLE_COPYING(lba_disk_extent_t);
};

#endif  // SERIALIZER_LOG_LBA_DISK_EXTENT_HPP_
