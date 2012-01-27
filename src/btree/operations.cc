#define __STDC_LIMIT_MACROS

#include "btree/operations.hpp"

#include <stdint.h>

#include "btree/slice.hpp"

real_superblock_t::real_superblock_t(buf_lock_t &sb_buf) {
    sb_buf_.swap(sb_buf);
}

void real_superblock_t::release() {
    sb_buf_.release_if_acquired();
}
void real_superblock_t::swap_buf(buf_lock_t &swapee) {
    sb_buf_.swap(swapee);
}

block_id_t real_superblock_t::get_root_block_id() const {
    rassert(sb_buf_.is_acquired());
    return reinterpret_cast<const btree_superblock_t *>(sb_buf_.const_buf()->get_data_read())->root_block;
}
void real_superblock_t::set_root_block_id(const block_id_t new_root_block) {
    rassert(sb_buf_.is_acquired());
    // We have to const_cast, because set_data unfortunately takes void* pointers, but get_data_read()
    // gives us const data. No way around this (except for making set_data take a const void * again, as it used to be).
    sb_buf_->set_data(const_cast<block_id_t *>(&reinterpret_cast<const btree_superblock_t *>(sb_buf_.buf()->get_data_read())->root_block), &new_root_block, sizeof(new_root_block));
}

void real_superblock_t::set_eviction_priority(int eviction_priority) {
    rassert(sb_buf_.is_acquired());
    sb_buf_->set_eviction_priority(eviction_priority);

}

int real_superblock_t::get_eviction_priority() {
    rassert(sb_buf_.is_acquired());
    return sb_buf_->get_eviction_priority();
}


bool find_superblock_metainfo_entry(char *beg, char *end, const std::vector<char> &key, char **verybeg_ptr_out,  uint32_t **size_ptr_out, char **beg_ptr_out, char **end_ptr_out) {
    while (beg < end) {
        char *verybeg_ptr = beg;

        uint32_t keysize;
        rassert(end - beg >= int(sizeof(keysize)));
        if (end - beg < int(sizeof(keysize) + key.size())) {
            return false;
        }

        keysize = *reinterpret_cast<uint32_t *>(beg);
        beg += sizeof(keysize);

        char *keybeg = beg;

        rassert(end - beg >= int64_t(keysize));
        beg += keysize;
        uint32_t valuesize;
        rassert(end - beg >= int(sizeof(valuesize)));
        uint32_t *valuesize_ptr = reinterpret_cast<uint32_t *>(beg);
        valuesize = *valuesize_ptr;
        beg += sizeof(valuesize);
        rassert(end - beg >= int64_t(valuesize));
        char *beg_ptr = beg;
        beg += valuesize;

        if (keysize == key.size() && std::equal(key.data(), key.data() + key.size(), keybeg)) {
            *verybeg_ptr_out = verybeg_ptr;
            *size_ptr_out = valuesize_ptr;
            *beg_ptr_out = beg_ptr;
            *end_ptr_out = beg;

            return true;
        }
    }

    return false;
}

bool get_superblock_metainfo(transaction_t *txn, buf_t *superblock, const std::vector<char> &key, std::vector<char> &value_out) {
    const btree_superblock_t *data = static_cast<const btree_superblock_t *>(superblock->get_data_read());

    // The const cast is okay because we access the data with rwi_read
    // and don't write to the blob.
    blob_t blob(const_cast<char *>(data->metainfo_blob), btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

    blob_acq_t acq;
    buffer_group_t group;
    blob.expose_all(txn, rwi_read, &group, &acq);

    int64_t group_size = group.get_size();
    std::vector<char> metainfo(group_size);

    buffer_group_t group_cpy;
    group_cpy.add_buffer(group_size, metainfo.data());

    buffer_group_copy_data(&group_cpy, const_view(&group));

    uint32_t *size;
    char *verybeg, *info_begin, *info_end;
    if (find_superblock_metainfo_entry(metainfo.data(), metainfo.data() + metainfo.size(), key, &verybeg, &size, &info_begin, &info_end)) {
        value_out.assign(info_begin, info_end);
        return true;
    } else {
        return false;
    }
}

void set_superblock_metainfo(transaction_t *txn, buf_t *superblock, const std::vector<char> &key, const std::vector<char> &value) {
    btree_superblock_t *data = static_cast<btree_superblock_t *>(superblock->get_data_major_write());

    blob_t blob(data->metainfo_blob, btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

    std::vector<char> metainfo;

    {
        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(txn, rwi_read, &group, &acq);

        int64_t group_size = group.get_size();
        metainfo.resize(group_size);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(group_size, metainfo.data());

        buffer_group_copy_data(&group_cpy, const_view(&group));
    }

    blob.clear(txn);

    uint32_t *size;
    char *verybeg, *info_begin, *info_end;
    if (find_superblock_metainfo_entry(metainfo.data(), metainfo.data() + metainfo.size(), key, &verybeg, &size, &info_begin, &info_end)) {
        std::vector<char>::iterator beg = metainfo.begin() + (info_begin - metainfo.data());
        std::vector<char>::iterator end = metainfo.begin() + (info_end - metainfo.data());
        // We must modify *size first because resizing the vector invalidates the pointer.
        rassert(value.size() <= UINT32_MAX);
        *size = value.size();

        std::vector<char>::iterator p = metainfo.erase(beg, end);

        metainfo.insert(p, value.begin(), value.end());
    } else {
        union {
            char x[sizeof(uint32_t)];
            uint32_t y;
        } data;
        rassert(value.size() < UINT32_MAX);
        data.y = value.size();

        metainfo.insert(metainfo.end(), data.x, data.x + sizeof(uint32_t));
        metainfo.insert(metainfo.end(), value.begin(), value.end());
    }

    blob.append_region(txn, metainfo.size());

    {
        blob_acq_t acq;
        buffer_group_t write_group;
        blob.expose_all(txn, rwi_write, &write_group, &acq);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(metainfo.size(), metainfo.data());

        buffer_group_copy_data(&write_group, const_view(&group_cpy));
    }
}



void delete_superblock_metainfo(transaction_t *txn, buf_t *superblock, const std::vector<char> &key) {
    btree_superblock_t *data = static_cast<btree_superblock_t *>(superblock->get_data_major_write());

    blob_t blob(data->metainfo_blob, btree_superblock_t::METAINFO_BLOB_MAXREFLEN);

    std::vector<char> metainfo;

    {
        blob_acq_t acq;
        buffer_group_t group;
        blob.expose_all(txn, rwi_read, &group, &acq);

        int64_t group_size = group.get_size();
        metainfo.resize(group_size);

        buffer_group_t group_cpy;
        group_cpy.add_buffer(group_size, metainfo.data());

        buffer_group_copy_data(&group_cpy, const_view(&group));
    }

    blob.clear(txn);

    uint32_t *size;
    char *verybeg, *info_begin, *info_end;
    bool found = find_superblock_metainfo_entry(metainfo.data(), metainfo.data() + metainfo.size(), key, &verybeg, &size, &info_begin, &info_end);

    rassert(found);

    if (found) {

        std::vector<char>::iterator p = metainfo.begin() + (verybeg - metainfo.data());
        std::vector<char>::iterator q = metainfo.begin() + (info_end - metainfo.data());
        metainfo.erase(p, q);

        {
            blob_acq_t acq;
            buffer_group_t write_group;
            blob.expose_all(txn, rwi_write, &write_group, &acq);

            buffer_group_t group_cpy;
            group_cpy.add_buffer(metainfo.size(), metainfo.data());

            buffer_group_copy_data(&write_group, const_view(&group_cpy));
        }
    }
}
