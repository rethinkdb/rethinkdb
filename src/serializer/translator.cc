#include "serializer/translator.hpp"

ser_block_id_t translator_serializer_t::translate_block_id(ser_block_id_t id, int mod_count, int mod_id, ser_block_id_t min) {
    return id * mod_count + mod_id + min;
}

int translator_serializer_t::untranslate_block_id(ser_block_id_t inner_id, int mod_count, ser_block_id_t min) {
    // We know that inner_id == id * mod_count + mod_id + min.
    // Thus inner_id - min == id * mod_count + mod_id.
    // It follows that inner_id - min === mod_id (modulo mod_count).
    // So (inner_id - min) % mod_count == mod_id (since 0 <= mod_id < mod_count).
    // (And inner_id - min >= 0, so '%' works as expected.)
    return (inner_id - min) % mod_count;
}

ser_block_id_t translator_serializer_t::xlate(ser_block_id_t id) {
    return translate_block_id(id, mod_count, mod_id, min);
}

translator_serializer_t::translator_serializer_t(serializer_t *inner, int mod_count, int mod_id, ser_block_id_t min)
    : inner(inner), mod_count(mod_count), mod_id(mod_id), min(min) {
    assert(mod_count > 0);
    assert(mod_id >= 0);
    assert(mod_id < mod_count);
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

bool translator_serializer_t::do_read(ser_block_id_t block_id, void *buf, read_callback_t *callback) {
    return inner->do_read(xlate(block_id), buf, callback);
}

bool translator_serializer_t::do_write(write_t *writes, int num_writes, write_txn_callback_t *callback) {
    write_t writes2[num_writes];
    for (int i = 0; i < num_writes; i++) {
        writes2[i].block_id = xlate(writes[i].block_id);
        writes2[i].buf = writes[i].buf;
        writes2[i].callback = writes[i].callback;
    }
    return inner->do_write(writes2, num_writes, callback);
}


size_t translator_serializer_t::get_block_size() {
    return inner->get_block_size();
}

ser_block_id_t translator_serializer_t::max_block_id() {
    int64_t x = inner->max_block_id() - min;
    if (x <= 0) {
        x = 0;
    } else {
        while (x % mod_count != mod_id) x++;
        x /= mod_count;
    }
    assert(xlate(x) >= inner->max_block_id());
    return x;
}


bool translator_serializer_t::block_in_use(ser_block_id_t id) {
    return inner->block_in_use(xlate(id));
}
