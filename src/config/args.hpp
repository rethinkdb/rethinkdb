
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
#define VERSION_STRING "0.2"

/**
 * Basic configuration parameters.
 * TODO: Many of these should be runtime switches.
 */
// Max concurrent IO requests per event queue
#define MAX_CONCURRENT_IO_REQUESTS                128

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
#define DEFAULT_EXTENT_SIZE                       (8 * MEGABYTE)

// Max number of blocks which can be read ahead in one i/o transaction (if enabled)
#define MAX_READ_AHEAD_BLOCKS 32

// Max size of log file name
#define MAX_LOG_FILE_NAME                         1024

// Max length of log message, including terminating \0
#define MAX_LOG_MSGLEN                            1024

// Queue ID of logging worker
#define LOG_WORKER 0

// Ratio of free ram to use for the cache by default
#define DEFAULT_MAX_CACHE_RATIO                   0.8f

// Maximum number of threads we support
// TODO: make this dynamic where possible
#define MAX_THREADS                                  128

// Maximum slices total
#define MAX_SLICES                                128

// Maximum number of files we use
#define MAX_SERIALIZERS                           32

// The number of ways we split a BTree (the most optimal is the number
// of cores, but we use a higher split factor to allow upgrading to
// more cores without migrating the database file).
#define DEFAULT_BTREE_SHARD_FACTOR                64

// The size allocated to the on-disk diff log for newly created databases
#ifdef NDEBUG
#define DEFAULT_PATCH_LOG_SIZE                     (512 * MEGABYTE)
#else
#define DEFAULT_PATCH_LOG_SIZE                     (4 * MEGABYTE)
#endif

// Default port to listen on
#define DEFAULT_LISTEN_PORT                       11211

// Default port to do replication on
#define DEFAULT_REPLICATION_PORT                  11319

// Default extension for the semantic file which is appended to the database name
#define DEFAULT_SEMANTIC_EXTENSION                ".semantic"

// Ticks (in milliseconds) the internal timed tasks are performed at
#define TIMER_TICKS_IN_MS                         5

// How many milliseconds to allow changes to sit in memory before flushing to disk
#define DEFAULT_FLUSH_TIMER_MS                    1000

// flush_waiting_threshold is the maximal number of transactions which can wait
// for a sync before a flush gets triggered on any single slice. As transactions only wait for
// sync with wait_for_flush enabled, this option plays a role only then.
#define DEFAULT_FLUSH_WAITING_THRESHOLD           8

// If wait_for_flush is true, concurrent flushing can be used to reduce the latency
// of each single flush. max_concurrent_flushes controls how many flushes can be active
// on a specific slice at any given time.
#define DEFAULT_MAX_CONCURRENT_FLUSHES            1

// If the size of the data affected by the current set of patches in a block is larger than
// block size / MAX_PATCHES_SIZE_RATIO, we flush the block instead of waiting for
// more patches to come.
// Note: An average write transaction under canonical workload leads to patches of about 75
// bytes of affected data.
// The actual value is continuously adjusted between MAX_PATCHES_SIZE_RATIO_MIN and
// MAX_PATCHES_SIZE_RATIO_MAX depending on how much the system is i/o bound
#define MAX_PATCHES_SIZE_RATIO_MIN                100
#define MAX_PATCHES_SIZE_RATIO_MAX                2
#define MAX_PATCHES_SIZE_RATIO_DURABILITY         5
#define RAISE_PATCHES_RATIO_AT_FRACTION_OF_UNSAVED_DATA_LIMIT 0.6

// If more than this many bytes of dirty data accumulate in the cache, then write
// transactions will be throttled.
// A value of 0 means that it will automatically be set to MAX_UNSAVED_DATA_LIMIT_FRACTION
// times the max cache size
#define DEFAULT_UNSAVED_DATA_LIMIT                0

// The unsaved data limit cannot exceed this fraction of the max cache size
#define MAX_UNSAVED_DATA_LIMIT_FRACTION           0.9

// We start flushing dirty pages as soon as we hit this fraction of the unsaved data limit
#define FLUSH_AT_FRACTION_OF_UNSAVED_DATA_LIMIT   0.9

// How many times the page replacement algorithm tries to find an eligible page before giving up.
// Note that (MAX_UNSAVED_DATA_LIMIT_FRACTION ** PAGE_REPL_NUM_TRIES) is the probability that the
// page replacement algorithm will succeed on a given try, and if that probability is less than 1/2
// then the page replacement algorithm will on average be unable to evict pages from the cache.
#define PAGE_REPL_NUM_TRIES                       10

// How large can the key be, in bytes?  This value needs to fit in a byte.
#define MAX_KEY_SIZE                              250

// Any values of this size or less will be directly stored in btree leaf nodes.
// Values greater than this size will be stored in overflow blocks. This value
// needs to fit in a byte.
#define MAX_IN_NODE_VALUE_SIZE                    250

// In addition to the value itself we could potentially store
// memcached flags, exptime, and a CAS value in the value contents, so
// we reserve space for that.
#define MAX_BTREE_VALUE_AUXILIARY_SIZE            (sizeof(btree_value) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t))
#define MAX_BTREE_VALUE_SIZE                      (MAX_BTREE_VALUE_AUXILIARY_SIZE + MAX_IN_NODE_VALUE_SIZE)

// memcached specifies the maximum value size to be 1MB
#define MAX_VALUE_SIZE                            MEGABYTE

// Values larger than this will be streamed in a set operation.
#define MAX_BUFFERED_SET_SIZE                     32768

// Values larger than this will be streamed in a get operation
#define MAX_BUFFERED_GET_SIZE                     32768

// If a single connection sends this many 'noreply' commands, the next command will
// have to wait until the first one finishes
#define MAX_CONCURRENT_QUERIES_PER_CONNECTION     500

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
#define MAX_GC_HIGH_RATIO                         0.990001

// The ratio at which we don't want to keep GC'ing.
#define DEFAULT_GC_LOW_RATIO                      0.5
#define MIN_GC_LOW_RATIO                          0.099999


// What's the maximum number of "young" extents we can have?
#define GC_YOUNG_EXTENT_MAX_SIZE                  50
// What's the definition of a "young" extent in microseconds?
#define GC_YOUNG_EXTENT_TIMELIMIT_MICROS          50000

// If the size of the LBA on a given disk exceeds LBA_MIN_SIZE_FOR_GC, then the fraction of the
// entries that are live and not garbage should be at least LBA_MIN_UNGARBAGE_FRACTION.
#define LBA_MIN_SIZE_FOR_GC                       (MEGABYTE * 20)
#define LBA_MIN_UNGARBAGE_FRACTION                0.15

// How many LBA structures to have for each file
#define LBA_SHARD_FACTOR                          16

// How many bytes of buffering space we can use per disk when reading the LBA. If it's set
// too high, then RethinkDB will eat a lot of memory at startup. This is bad because tcmalloc
// doesn't return memory to the OS. If it's set too low, startup will take a longer time.
#define LBA_READ_BUFFER_SIZE                      GIGABYTE

// How many different places in each file we should be writing to at once, not counting the
// metablock or LBA
#define MAX_ACTIVE_DATA_EXTENTS                   64
#define DEFAULT_ACTIVE_DATA_EXTENTS               1

// How many zones the serializer will divide a block device into
#define DEFAULT_FILE_ZONE_SIZE                    GIGABYTE
#define MAX_FILE_ZONES                            (TERABYTE / DEFAULT_FILE_ZONE_SIZE)

// XXX: I increased this from 65536; make sure it's actually needed.
#define COROUTINE_STACK_SIZE                      131072


// TODO: It would be nice if we didn't need MAX_HOSTNAME_LEN and
// MAX_PATH_LEN.. just because we're storing stuff in the database.

// Maximum length of a hostname we communicate with
#define MAX_HOSTNAME_LEN                          100

//max length of a path that we have to store during run time
#define MAX_PATH_LEN                              200

// Size of a cache line (used in cache_line_padded_t).
#define CACHE_LINE_SIZE                           64

#endif // __CONFIG_ARGS_H__

