
#ifndef __IN_PLACE_SERIALIZER_HPP__
#define __IN_PLACE_SERIALIZER_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "arch/resource.hpp"
#include "async_io.hpp"
#include "btree/admin.hpp"

// TODO: what about multiple modifications to the tree that need to be
// performed atomically?

// TODO: a robust implementation requires a replay log.

// TODO: how to maintain the id of the root block?

/* This is a serializer that writes blocks in place. It should be
 * efficient for rotational drives and flash drives with a very good
 * FTL. It's also a good sanity check that the rest of the system
 * isn't tightly coupled with a log-structured serializer. */
template<class config_t>
struct in_place_serializer_t {
public:
    typedef off64_t block_id_t;
    typedef typename config_t::btree_admin_t btree_admin_t;

public:
    explicit in_place_serializer_t(size_t _block_size)
        : dbfd(-1), dbsize(-1), block_size(_block_size)
        {}
    ~in_place_serializer_t() {
        check("Database file was not properly closed", dbfd != -1);
    }
    
    /* Initialize the serializer with the backing store. */
    void init(char *db_path) {
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
        
        // Leave space for the metablock if necessary
        if(dbsize == 0) {
            btree_admin_t::create_db(dbfd);
            dbsize = block_size;
        }
    }

    void close() {
        ::close(dbfd);
        dbfd = -1;
    }

public:
    
    /* Fires off an async request to read the block identified by
     * block_id into buf, associating state with the request. */
    void do_read(event_queue_t *queue, block_id_t block_id, void *buf,
                 void *state) {
        schedule_aio_read(dbfd, block_id, block_size, buf, queue, state);
    }
    
    /* Fires off an async request to write the block identified by
     * block_id into buf, associating state with the request. Returns
     * the new block id (in the case of the in_place_serializer, it is
     * always the same as the original id). If the block is new, NULL
     * can be passed in place of block_id, in which case the return
     * value will be the id of the newly written block. */
    block_id_t do_write(event_queue_t *queue, block_id_t block_id, void *buf,
                        void *state) {
        aio_write_t aio_writes[1];

        aio_writes[0].resource = dbfd;
        aio_writes[0].offset = block_id;
        aio_writes[0].length = block_size;
        aio_writes[0].buf = buf;
        aio_writes[0].state = state;

        schedule_aio_write(aio_writes, 1, queue);

        return block_id;
    }

    struct write {
        block_id_t  block_id;
        void        *buf;
        void        *state;
    };

    void do_write(event_queue_t *queue, write *writes, int num_writes) {
        aio_write_t aio_writes[num_writes];
        int i;
        for (i = 0; i < num_writes; i++) {
            aio_writes[i].resource = dbfd;
            aio_writes[i].offset = writes[i].block_id;
            aio_writes[i].length = block_size;
            aio_writes[i].buf = writes[i].buf;
            aio_writes[i].state = writes[i].state;
        }

        schedule_aio_write(aio_writes, num_writes, queue);

    }

public:
    /* Returns true iff block_id is NULL. */
    static bool is_block_id_null(block_id_t block_id) {
        return block_id == -1;
    }

    /* Generates a unique block id. */
    block_id_t gen_block_id() {
        off64_t new_block_id = dbsize;
        dbsize += block_size;
        return new_block_id;
    }

    /* Consumer of the serializer can store bootstrapping information
     * in the superblock. For the in place serializer the superblock
     * is always at the beginning of the file. */
    block_id_t get_superblock_id() {
        return 0;
    }

public:
    resource_t dbfd;
    off64_t dbsize;
    size_t block_size;
    static const block_id_t null_block_id = -1;
};

#endif // __IN_PLACE_SERIALIZER_HPP__

