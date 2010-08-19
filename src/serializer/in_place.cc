#include "serializer/in_place.hpp"

#include <sys/types.h>
#include "utils.hpp"
#include "config/cmd_args.hpp"
#include "config/code.hpp"
#include "arch/io.hpp"
#include "corefwd.hpp"
#include "cpu_context.hpp"
#include "event_queue.hpp"

in_place_serializer_t::in_place_serializer_t(char *_db_path, size_t _block_size)
    : block_size(_block_size), state(state_unstarted), dbfd(INVALID_FD), dbsize(-1) {
    assert(strlen(_db_path) <= MAX_DB_FILE_NAME - 1);   // "- 1" for the null-terminator
    strncpy(db_path, _db_path, MAX_DB_FILE_NAME);
}

in_place_serializer_t::~in_place_serializer_t() {
    assert(state == state_unstarted || state == state_shut_down);
}

bool in_place_serializer_t::start(ready_callback_t *cb) {

    assert(state == state_unstarted);
    
    // Open the DB file
    dbfd = open(db_path,
                O_RDWR | O_CREAT | O_DIRECT | O_LARGEFILE | O_NOATIME,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    check("Could not open database file", dbfd == -1);
    
    // Determine last block id
    dbsize = lseek64(dbfd, 0, SEEK_END);
    check("Could not determine database file size", dbsize == -1);
    off64_t res = lseek64(dbfd, 0, SEEK_SET);
    check("Could not reset database file position", res == -1);
    
    // Create an initial superblock if necessary
    if (dbsize == 0) {
        
        dbsize += block_size;
        
        // ftruncate() will initialize the superblock to 0
        int res = ftruncate(dbfd, block_size);
        check("Could not initialize superblock via ftruncate()", res != 0);
    }
    
    state = state_ready;
    return true;
}

bool in_place_serializer_t::do_read(block_id_t block_id, void *buf, read_callback_t *callback) {
    
    assert(state == state_ready);
    
    event_queue_t *queue = get_cpu_context()->event_queue;
    queue->iosys.schedule_aio_read(
        dbfd,
        id_to_offset(block_id),
        block_size,
        buf,
        queue,
        callback);
    
    // Reads never complete immediately
    return false;
}

bool in_place_serializer_t::do_write(write_t *writes, int num_writes, write_txn_callback_t *callback) {
    
    assert(state == state_ready);
    
    // TODO: watch how we're allocating
    int num_actual_writes = 0;   // How many writes there are if you don't count deletions
    for (int i = 0; i < num_writes; i++) {
        
        if (writes[i].buf) {
        
            assert(writes[i].callback->associated_txn == NULL);   // Illegal to reuse a callback
            writes[i].callback->associated_txn = callback;
            
            event_queue_t *queue = get_cpu_context()->event_queue;
            queue->iosys.schedule_aio_write(
                dbfd,
                id_to_offset(writes[i].block_id),
                block_size,
                writes[i].buf,
                queue,
                writes[i].callback);
        
        } else {
            
            // It's a deletion. Do nothing because we don't support deletions.
            assert(!writes[i].callback);
        }
    }
    
    assert(callback->blocks_outstanding == 0);   // Illegal to reuse a callback
    callback->blocks_outstanding = num_actual_writes;
    
    return (num_actual_writes == 0);
}

block_id_t in_place_serializer_t::gen_block_id() {
    assert(dbsize != 0);   // Superblock ID is reserved
    off64_t new_block_id = dbsize / block_size;
    dbsize += block_size;
    return new_block_id;
}

bool in_place_serializer_t::shutdown(shutdown_callback_t *cb) {
    assert(state == state_starting_up || state == state_ready);
    
    ::close(dbfd);
    
    state = state_shut_down;
    return true;
}

off64_t in_place_serializer_t::id_to_offset(block_id_t id) {
    return id * block_size;
}