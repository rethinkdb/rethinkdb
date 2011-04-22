#ifndef __REPLICATION_DELETE_QUEUE_HPP__
#define __REPLICATION_DELETE_QUEUE_HPP__

#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/transactor.hpp"

/*
                       D E L E T E   Q U E U E

("thwommm thwommmm...")

                             What to do?

Keep track of what we've deleted from a given timestamp.  We want to
know what deletes happened with timestamp t1 and later.  Then, when
backfilling from time t1, we send all the deletes, and then we send
the current state of the tree, and then we send the latest changes
that happened after that.  (Of course we kind of send them all at the
same time, and the slave sorts them out.)

                          How to implement?

The delete queue is just a long chain of btree_keys.  We store an
array of offsets into the delete queue corresponding to times t_i
... t_j that tell when to start reading from the delete queue for
timestamp t_i, relative to some fixed offset.  The thing is, we keep
deleting old information from the old end (the front) of the queue.
So we also keep an offset tracking the total amount deleted from the
front of the queue.

So the delete queue consists of one off64_t and two largebufs.  The
off64_t is the total-deleted-from-front offset, and one of the
largebufs is a concatenation of (timestamp,offset) pairs (12 bytes per
pair).  The timestamps and offsets are monotonic, but they don't have
to be continuous, in cases of exceeding low activity, or in cases
where the server was shut down and restarted more than a second later.
The other largebuf is a concatenation of btree_keys, and the first
largebuf's offsets, after subtracting the deleted-from-front offset,
refer to offsets within this second largebuf.  (They should always
point to boundaries between two btree_keys.)

The large buffers have weirdly sized large buf references.  The
"delete queue root node" exists at some block id and is as follows:

(8B) off64_t: the deleted-from-front offset (28B) large_buf_ref with
at most 3 inlined block ids: the (timestamp, offset) queue (4048B)
large_buf_ref with lots of room for inlined block ids

Maybe the (timestamp, offset) queue will have its parameters adjusted
after some testing.  3 inlined block ids is enough for a 1-level queue
to have 1000 seconds of delete queue

                         Where to implement?

To preserve ordering, so that the timestamps of delete messages are
monotonic, the delete queue must receive its delete messages from the
slice's thread, or be on the slice's thread itself.  Also, each slice
needs to manage its backfilling separately anyway, and this splits up
the work across cores.

*/

struct btree_key_t;
struct store_key_t;

namespace replication {

namespace delete_queue {

struct t_and_o {
    repli_timestamp timestamp;
    off64_t offset;
} __attribute__((__packed__));

}  // namespace delete_queue

struct delete_queue_block_t {
    block_magic_t magic;

    static const block_magic_t expected_magic;
};

void initialize_empty_delete_queue(delete_queue_block_t *dqb, block_size_t block_size);

// Instead of passing keys one by one, we just pass the buffers that
// contain all the keys.  These can then get passed in the same manner
// to the slave, without so much needless processing and reprocessing,
// unpacking and packing and unpacking and repacking.  The boundaries
// of the individual buffers will not lie on key boundaries.  The
// sequence of buffers contains a bunch of concatenated btree keys.
class deletion_key_stream_receiver_t {
public:
    virtual bool should_send_deletion_keys(bool can_send_deletion_keys) = 0;
    virtual void deletion_key(const btree_key_t *key) = 0;
    virtual void done_deletion_keys() = 0;
protected:
    virtual ~deletion_key_stream_receiver_t() { }
};

// Acquires a delete queue, appends a (timestamp, key) pair to the
// queue, and releases the queue.  The delete queue is identified by
// queue_root.  This must be called on the transaction's home thread.
void add_key_to_delete_queue(int64_t delete_queue_limit, boost::shared_ptr<transactor_t>& txor, block_id_t queue_root, repli_timestamp timestamp, const store_key_t *key);

// Dumps keys from the delete queue, blocking until all the keys in
// the interval have been passed to the recipient.  All keys whose
// timestamps are grequal to the begin_timestamp and less than the
// end_timestamp are passed to recipient, in no particular order.
// TODO: Is there any reason for an end_timestamp parameter?
void dump_keys_from_delete_queue(boost::shared_ptr<transactor_t>& txor, block_id_t queue_root, repli_timestamp begin_timestamp, deletion_key_stream_receiver_t *recipient);




}  // namespace replication

#endif  // __REPLICATION_DELETE_QUEUE_HPP__
