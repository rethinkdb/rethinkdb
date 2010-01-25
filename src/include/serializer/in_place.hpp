
#ifndef __IN_PLACE_SERIALIZER_HPP__
#define __IN_PLACE_SERIALIZER_HPP__

// TODO: what about multiple modifications to the tree that need to be
// performed atomically?

// TODO: what about maintaining information about the id/address of
// the "master" block? Should this be done by the serializer or by
// someone else? How could someone else do it for a log-structured
// serializer without virtual addressing?

// TODO: a robust implementation requires a replay log.

/* This is a serializer that writes blocks in place. It should be
 * efficient for rotational drives and flash drives with a very good
 * FTL. It's also a good sanity check that the rest of the system
 * isn't tightly coupled with a log-structured serializer. */
struct in_place_serializer_t {
public:
    typedef off64_t block_id_t;

public:
    /* Fires off an async request to read the block identified by
     * block_id into buf, associating state with the request. */
    void do_read(block_id_t block_id, void *buf, void *state) {
    }
    
    /* Fires off an async request to write the block identified by
     * block_id into buf, associating state with the request. Returns
     * the new block id (in the case of the in_place_serializer, it is
     * always the same as the original id). If the block is new, NULL
     * can be passed in place of block_id, in which case the return
     * value will be the id of the newly written block. */
    block_id_t do_write(block_id_t block_id, void *buf, void *state) {
    }

    /* Returns true iff block_id is NULL. */
    bool is_block_id_null(block_id_t block_id) {
        return block_id == -1;
    }

    /* Generates a unique block id. */
    block_id_t gen_block_id() {
        return 0;
    }
};

#endif // __IN_PLACE_SERIALIZER_HPP__

