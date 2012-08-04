#include "serializer/log/lba/disk_extent.hpp"

#include "arch/arch.hpp"

lba_disk_extent_t::lba_disk_extent_t(extent_manager_t *_em, direct_file_t *file, file_account_t *io_account)
  : em(_em), data(new extent_t(em, file)), offset(data->offset), count(0)
{
    // Make sure that the size of the header is a multiple of the size of one entry, so that the
    // header doesn't prevent the entries from aligning with DEVICE_BLOCK_SIZE.
    rassert(divides(sizeof(lba_entry_t), offsetof(lba_extent_t, entries[0])));
    CT_ASSERT(offsetof(lba_extent_t, entries[0]) == sizeof(lba_extent_t::header_t));

    lba_extent_t::header_t header;
    bzero(&header, sizeof(header));
    memcpy(header.magic, lba_magic, LBA_MAGIC_SIZE);
    data->append(&header, sizeof(header), io_account);
}

lba_disk_extent_t::lba_disk_extent_t(extent_manager_t *_em, direct_file_t *file, off64_t _offset, int _count)
    : em(_em), data(new extent_t(em, file, _offset, offsetof(lba_extent_t, entries[0]) + sizeof(lba_entry_t) * _count)), offset(_offset), count(_count)
{
}


void lba_disk_extent_t::add_entry(lba_entry_t entry, file_account_t *io_account) {
    // Make sure that entries will align with DEVICE_BLOCK_SIZE

    // Make sure that there is room
    rassert(data->amount_filled + sizeof(lba_entry_t) <= em->extent_size);

    data->append(&entry, sizeof(lba_entry_t), io_account);
    ++count;
}

void lba_disk_extent_t::sync(file_account_t *io_account, extent_t::sync_callback_t *cb) {
    while (data->amount_filled % DEVICE_BLOCK_SIZE != 0) {
        add_entry(lba_entry_t::make_padding_entry(), io_account);
    }

    data->sync(cb);
}

void lba_disk_extent_t::read_step_1(read_info_t *info_out, extent_t::read_callback_t *cb) {
    info_out->buffer = malloc_aligned(em->extent_size, DEVICE_BLOCK_SIZE);
    info_out->count = count;
    data->read(0, sizeof(lba_extent_t) + sizeof(lba_entry_t) * count, info_out->buffer, cb);
}

void lba_disk_extent_t::read_step_2(read_info_t *info, in_memory_index_t *index) {
    lba_extent_t *extent = reinterpret_cast<lba_extent_t *>(info->buffer);
    rassert(memcmp(extent->header.magic, lba_magic, LBA_MAGIC_SIZE) == 0);

    for (int i = 0; i < info->count; ++i) {
        lba_entry_t *e = &extent->entries[i];
        if (!lba_entry_t::is_padding(e)) {
            index->set_block_info(e->block_id, e->recency, e->offset);
        }
    }

    free(info->buffer);
}

