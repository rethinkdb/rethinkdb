// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/log/lba/disk_extent.hpp"

#include "arch/arch.hpp"
#include "math.hpp"

lba_disk_extent_t::lba_disk_extent_t(extent_manager_t *_em, file_t *file, file_account_t *io_account)
    : em(_em), data(new extent_t(em, file)), count(0) {
    em->assert_thread();

    // Make sure that the size of the header is a multiple of the size of one entry, so that the
    // header doesn't prevent the entries from aligning with DEVICE_BLOCK_SIZE.
    rassert(divides(sizeof(lba_entry_t), offsetof(lba_extent_t, entries[0])));
    rassert(offsetof(lba_extent_t, entries[0]) == sizeof(lba_extent_t::header_t));

    lba_extent_t::header_t header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, lba_magic, LBA_MAGIC_SIZE);
    data->append(&header, sizeof(header), io_account);
}

lba_disk_extent_t::lba_disk_extent_t(extent_manager_t *_em, file_t *file, int64_t _offset, int _count)
    : em(_em), data(new extent_t(em, file, _offset, offsetof(lba_extent_t, entries[0]) + sizeof(lba_entry_t) * _count)), count(_count) {
    em->assert_thread();
}


void lba_disk_extent_t::add_entry(lba_entry_t entry, file_account_t *io_account) {
    em->assert_thread();
    // Make sure that entries will align with DEVICE_BLOCK_SIZE

    // Make sure that there is room
    rassert(data->amount_filled + sizeof(lba_entry_t) <= em->extent_size);

    data->append(&entry, sizeof(lba_entry_t), io_account);
    count++;
}

void lba_disk_extent_t::sync(file_account_t *io_account, extent_t::sync_callback_t *cb) {
    em->assert_thread();
    while (data->amount_filled % DEVICE_BLOCK_SIZE != 0) {
        add_entry(lba_entry_t::make_padding_entry(), io_account);
    }

    data->sync(cb);
}

void lba_disk_extent_t::read_step_1(read_info_t *info_out, extent_t::read_callback_t *cb) {
    em->assert_thread();
    info_out->buffer = scoped_device_block_aligned_ptr_t<lba_extent_t>(em->extent_size);
    info_out->count = count;
    data->read(0, sizeof(lba_extent_t) + sizeof(lba_entry_t) * count, info_out->buffer.get(), cb);
}

void lba_disk_extent_t::read_step_2(read_info_t *info, in_memory_index_t *index) {
    em->assert_thread();
    lba_extent_t *extent = info->buffer.get();
    guarantee(memcmp(extent->header.magic, lba_magic, LBA_MAGIC_SIZE) == 0);

    for (int i = 0; i < info->count; i++) {
        lba_entry_t *e = &extent->entries[i];
        if (!lba_entry_t::is_padding(e)) {
            // The on-disk format still stores 32 bit block sizes.
            // We've never actually used them, and we now use 16 bit block sizes
            // for the in-memory index to save a few bytes.
            guarantee(e->ser_block_size <= std::numeric_limits<uint16_t>::max());
            index->set_block_info(e->block_id, e->recency, e->offset,
                                  static_cast<uint16_t>(e->ser_block_size));
        }
    }

    info->buffer.reset();
}

