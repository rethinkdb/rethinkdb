#include "serializer/in_place.hpp"

#include <sys/types.h>
#include "utils.hpp"
#include "config/cmd_args.hpp"
#include "config/alloc.hpp"
#include "corefwd.hpp"
#include "arch/arch.hpp"

in_place_serializer_t::in_place_serializer_t(char *_db_path, size_t _block_size)
    : block_size(_block_size), state(state_unstarted), dbfile(NULL), dbsize(-1) {
    assert(strlen(_db_path) <= MAX_DB_FILE_NAME - 1);   // "- 1" for the null-terminator
    strncpy(db_path, _db_path, MAX_DB_FILE_NAME);
}

in_place_serializer_t::~in_place_serializer_t() {
    assert(state == state_unstarted || state == state_shut_down);
}

bool in_place_serializer_t::start(ready_callback_t *cb) {

    assert(state == state_unstarted);
    
    // Open the DB file
    
    dbfile = new direct_file_t(db_path, direct_file_t::mode_read|direct_file_t::mode_write);
    
    // Determine last block id
    dbsize = dbfile->get_size();
    
    // Create an initial superblock if necessary
    if (dbsize == 0) {
        
        dbsize += block_size;
        dbfile->set_size(dbsize);
        
        void *zeroes = malloc(block_size);
        bzero(zeroes, block_size);
        dbfile->write_blocking(0, block_size, zeroes);
        free(zeroes);
    }
    
    state = state_ready;
    return true;
}

bool in_place_serializer_t::do_read(ser_block_id_t block_id, void *buf, read_callback_t *callback) {
    
    assert(state == state_ready);
    
    return dbfile->read_async(id_to_offset(block_id), block_size, buf, callback);
}

bool in_place_serializer_t::do_write(write_t *writes, int num_writes, write_txn_callback_t *callback) {
    
    assert(state == state_ready);
    
    // TODO: watch how we're allocating
    int num_actual_writes = 0;   // How many writes there are if you don't count deletions
    for (int i = 0; i < num_writes; i++) {
        
        if (writes[i].buf) {
        
            assert(writes[i].callback->associated_txn == NULL);   // Illegal to reuse a callback
            writes[i].callback->associated_txn = callback;
            
            bool done __attribute__((unused)) = dbfile->write_async(
                id_to_offset(writes[i].block_id), block_size,
                writes[i].buf,
                writes[i].callback);
            assert(!done);   // We assume writes never complete immediately
        
        } else {
            
            // It's a deletion. Do nothing because we don't support deletions.
            assert(!writes[i].callback);
        }
    }
    
    assert(callback->blocks_outstanding == 0);   // Illegal to reuse a callback
    callback->blocks_outstanding = num_actual_writes;
    
    return (num_actual_writes == 0);
}

ser_block_id_t in_place_serializer_t::gen_block_id() {
    assert(dbsize != 0);   // Superblock ID is reserved
    off64_t new_block_id = dbsize / block_size;
    dbsize += block_size;
    dbfile->set_size(dbsize);
    return new_block_id;
}

bool in_place_serializer_t::shutdown(shutdown_callback_t *cb) {
    assert(state == state_starting_up || state == state_ready);
    
    delete dbfile;
    
    state = state_shut_down;
    return true;
}

off64_t in_place_serializer_t::id_to_offset(ser_block_id_t id) {
    return id * block_size;
}
