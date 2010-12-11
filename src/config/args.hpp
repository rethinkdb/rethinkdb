
#ifndef __CONFIG_ARGS_H__
#define __CONFIG_ARGS_H__

#define KILOBYTE 1024L
#define MEGABYTE (KILOBYTE*1024L)
#define GIGABYTE (MEGABYTE*1024L)
#define TERABYTE (GIGABYTE*1024L)

/*!
 * Version strings
 */

#define SOFTWARE_NAME_STRING "RethinkDB"
#define VERSION_STRING "0.0.1"

/**
 * Basic configuration parameters.
 * TODO: Many of these should be runtime switches.
 */
// Max concurrent IO requests per event queue
#define MAX_CONCURRENT_IO_REQUESTS                256

// Don't send more IO requests to the system until the per-thread
// queue of IO requests is higher than this depth
#define TARGET_IO_QUEUE_DEPTH                     128

// Defines the maximum size of the batch of IO events to process on
// each loop iteration. A larger number will increase throughput but
// decrease concurrency
#define MAX_IO_EVENT_PROCESSING_BATCH_SIZE        50

// Size of the buffer used to perform IO operations (in bytes).
#define IO_BUFFER_SIZE                            (4 * KILOBYTE)

// Size of the device block size (in bytes)
#define DEVICE_BLOCK_SIZE                         (4 * KILOBYTE)

// Size of each btree node (in bytes) on disk
#define DEFAULT_BTREE_BLOCK_SIZE                  (4 * KILOBYTE)

// Maximum number of data blocks
#define MAX_DATA_EXTENTS                          (TERABYTE / (16 * KILOBYTE))

// Size of each extent (in bytes)
#define DEFAULT_EXTENT_SIZE                       (1 * MEGABYTE)

// Max size of log file name
#define MAX_LOG_FILE_NAME                         1024

// Max length of log message
#define MAX_LOG_MSGLEN                            1024

// Queue ID of logging worker
#define LOG_WORKER 0

// Ratio of free ram to use for the cache by default
#define DEFAULT_MAX_CACHE_RATIO                   0.8f

// Maximum number of operations packed into a single request
// TODO: make this dynamic and get rid of this parameter
#define MAX_OPS_IN_REQUEST                        16

// Maximum number of CPUs we support
// TODO: make this dynamic where possible
#define MAX_CPUS                                  128

// Maximum slices total
#define MAX_SLICES                                128

// Maximum number of files we use
#define MAX_SERIALIZERS                           32

// The number of ways we split a BTree (the most optimal is the number
// of cores, but we use a higher split factor to allow upgrading to
// more cores without migrating the database file).
#define DEFAULT_BTREE_SHARD_FACTOR                16

// Default port to listen on
#define DEFAULT_LISTEN_PORT                       8080

// Default extension for the semantic file which is appended to the database name
#define DEFAULT_SEMANTIC_EXTENSION                ".semantic"

// Ticks (in milliseconds) the internal timed tasks are performed at
#define TIMER_TICKS_IN_MS                         5

// How many milliseconds to allow changes to sit in memory before flushing to disk
#define DEFAULT_FLUSH_TIMER_MS                    5000

// If the number of dirty buffers is more than X% of the maximum number of buffers allowed, then
// writeback will be started. DEFAULT_FLUSH_THRESHOLD_PERCENT is the default value of X.
#define DEFAULT_FLUSH_THRESHOLD_PERCENT           30

// How many times the page replacement algorithm tries to find an eligible page before giving up
#define PAGE_REPL_NUM_TRIES                       3

// How large can the key be, in bytes?  This value needs to fit in a byte.
#define MAX_KEY_SIZE                              250

// Any values of this size or less will be directly stored in btree leaf nodes.
// Values greater than this size will be stored in overflow blocks. This value
// needs to fit in a byte.
#define MAX_IN_NODE_VALUE_SIZE                    250

// In addition to the value itself we could potentially store memcached flags
// and a CAS value in the value contents, so we reserve space for that.
#define MAX_TOTAL_NODE_CONTENTS_SIZE              (MAX_IN_NODE_VALUE_SIZE + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint32_t))

// memcached specifies the maximum value size to be 1MB
#define MAX_VALUE_SIZE                            (1024 * KILOBYTE)

// Values larger than this will be streamed in a set operation. Must be smaller
// than the size of the conn_fsm's rbuf.
#define MAX_BUFFERED_SET_SIZE                     1000

// Values larger than this will be streamed in a get operation
#define MAX_BUFFERED_GET_SIZE                     10000

// How many timestamps we store in a leaf node.  We store the
// NUM_LEAF_NODE_EARLIER_TIMES+1 most-recent timestamps.
#define NUM_LEAF_NODE_EARLIER_TIMES               2

// Perform allocator GC every N milliseconds (the resolution is limited to TIMER_TICKS_IN_MS)
#define ALLOC_GC_INTERVAL_MS                      3000

//filenames for the database
#define DEFAULT_DB_FILE_NAME                      "rethinkdb_data"

// We assume there will never be more than this many blocks. The value
// is computed by dividing 1 TB by the smallest reasonable block size.
// This value currently fits in 32 bits, and so block_id_t and
// ser_block_id_t is a uint32_t.
#define MAX_BLOCK_ID                              (TERABYTE / KILOBYTE)

// We assume that there will never be more than this many blocks held in memory by the cache at
// any one time. The value is computed by dividing 50 GB by the smallest reasonable block size.
#define MAX_BLOCKS_IN_MEMORY                      (50 * GIGABYTE / KILOBYTE)

// This special block ID indicates the superblock. It doesn't really belong here because it's more
// of a magic constant than a tunable parameter.
#define SUPERBLOCK_ID                             0

// The ratio at which we should start GCing.
#define DEFAULT_GC_HIGH_RATIO                     0.65
#define MAX_GC_HIGH_RATIO                         0.99

// The ratio at which we don't want to keep GC'ing.
#define DEFAULT_GC_LOW_RATIO                      0.5
#define MIN_GC_LOW_RATIO                          0.01


// What's the maximum number of "young" extents we can have?
#define GC_YOUNG_EXTENT_MAX_SIZE                  50
// What's the definition of a "young" extent in microseconds?
#define GC_YOUNG_EXTENT_TIMELIMIT_MICROS          50000

// The ratio at which we should GC the lba list.
#define LBA_GC_THRESHOLD_RATIO_NUMERATOR          9
#define LBA_GC_THRESHOLD_RATIO_DENOMINATOR        10

// How many LBA structures to have for each file
#define LBA_SHARD_FACTOR                          16

// How many different places in each file we should be writing to at once, not counting the
// metablock or LBA
#define MAX_ACTIVE_DATA_EXTENTS                   64
#define DEFAULT_ACTIVE_DATA_EXTENTS               8

// How many zones the serializer will divide a block device into
#define DEFAULT_FILE_ZONE_SIZE                    GIGABYTE
#define MAX_FILE_ZONES                            (TERABYTE / DEFAULT_FILE_ZONE_SIZE)

#endif // __CONFIG_ARGS_H__

