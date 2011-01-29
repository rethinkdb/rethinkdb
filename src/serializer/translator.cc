#include "serializer/translator.hpp"

ser_block_id_t translator_serializer_t::translate_block_id(block_id_t id, int mod_count, int mod_id, config_block_id_t cfgid) {
    return ser_block_id_t::make(id * mod_count + mod_id + cfgid.subsequent_ser_id().value);
}

int translator_serializer_t::untranslate_block_id(ser_block_id_t inner_id, int mod_count, config_block_id_t cfgid) {
    // We know that inner_id == id * mod_count + mod_id + min.
    // Thus inner_id - min == id * mod_count + mod_id.
    // It follows that inner_id - min === mod_id (modulo mod_count).
    // So (inner_id - min) % mod_count == mod_id (since 0 <= mod_id < mod_count).
    // (And inner_id - min >= 0, so '%' works as expected.)
    return (inner_id.value - cfgid.subsequent_ser_id().value) % mod_count;
}

ser_block_id_t translator_serializer_t::xlate(block_id_t id) {
    return translate_block_id(id, mod_count, mod_id, cfgid);
}

translator_serializer_t::translator_serializer_t(serializer_t *inner_, int mod_count_, int mod_id_, config_block_id_t cfgid_)
    : inner(inner_), mod_count(mod_count_), mod_id(mod_id_), cfgid(cfgid_) {
    rassert(mod_count > 0);
    rassert(mod_id >= 0);
    rassert(mod_id < mod_count);
}

void *translator_serializer_t::malloc() {
    return inner->malloc();
}

void *translator_serializer_t::clone(void *data) {
    return inner->clone(data);
}

void translator_serializer_t::free(void *ptr) {
    inner->free(ptr);
}

bool translator_serializer_t::do_read(block_id_t block_id, void *buf, serializer_t::read_callback_t *callback, ser_transaction_id_t** transaction_id) {
    return inner->do_read(xlate(block_id), buf, callback, transaction_id);
}


bool translator_serializer_t::do_write(write_t *writes, int num_writes, serializer_t::write_txn_callback_t *callback) {
    std::vector<serializer_t::write_t> writes2;
    for (int i = 0; i < num_writes; i++) {
        writes2.push_back(serializer_t::write_t(xlate(writes[i].block_id), writes[i].recency_specified, writes[i].recency,
                                                writes[i].buf_specified, writes[i].buf, writes[i].callback));
    }
    return inner->do_write(writes2.data(), num_writes, callback);
}


block_size_t translator_serializer_t::get_block_size() {
    return inner->get_block_size();
}

block_id_t translator_serializer_t::max_block_id() {
    int64_t x = inner->max_block_id().value - cfgid.subsequent_ser_id().value;
    if (x <= 0) {
        x = 0;
    } else {
        while (x % mod_count != mod_id) x++;
        x /= mod_count;
    }
    rassert(xlate(x).value >= inner->max_block_id().value);

    while (x > 0) {
        --x;
        if (block_in_use(x)) {
            ++x;
            break;
        }
    }
    return x;
}


bool translator_serializer_t::block_in_use(block_id_t id) {
    return inner->block_in_use(xlate(id));
}

repli_timestamp translator_serializer_t::get_recency(block_id_t id) {
    return inner->get_recency(xlate(id));
}
