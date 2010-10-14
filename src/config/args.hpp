
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
#define VERSION_STRING "0.0.0"

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

// Defines the maximum number of allocators in
// dynamic_pool_alloc_t. Since the size of each allocator is doubled
// every time, a reasonably small number should be sufficient.
#define DYNAMIC_POOL_MAX_ALLOCS                   20

// Initial number of objects in the first dynamic pool allocator.
#define DYNAMIC_POOL_INITIAL_NOBJECTS             100

// Size of the buffer used to perform IO operations (in bytes).
#define IO_BUFFER_SIZE                            (4 * KILOBYTE)

// Size of the device block size (in bytes)
#define DEVICE_BLOCK_SIZE                         (4 * KILOBYTE)

//Size of the field on each block used to record block_id
#define BLOCK_META_DATA_SIZE                      8

// Size of each btree node (in bytes) on disk
#define BTREE_BLOCK_SIZE                          (4 * KILOBYTE)

//actual btree block size we have to work with
#define BTREE_USABLE_BLOCK_SIZE                   (BTREE_BLOCK_SIZE - BLOCK_META_DATA_SIZE)

// Maximum number of data blocks
#define MAX_DATA_EXTENTS                          (TERABYTE / EXTENT_SIZE)

// Size of each extent (in bytes)
// Value is very low for testing purposes.
#define EXTENT_SIZE                               (16 * KILOBYTE)

// Max size of database file name
#define MAX_DB_FILE_NAME                          1024

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
// TODO: when we build a real serializer, we should make this option
// configurable on a per-database level (with a sensible default), and
// provide a migration tool in case the customers upgrade to more
// cores than was originally anticipated.
#define BTREE_SHARD_FACTOR                        16

// Default port to listen on
#define DEFAULT_LISTEN_PORT                       8080

// Ticks (in milliseconds) the internal timed tasks are performed at
#define TIMER_TICKS_IN_MS                         5

// How many milliseconds to allow changes to sit in memory before flushing to disk
#define DEFAULT_FLUSH_TIMER_MS                    5000

// If the number of dirty buffers is more than X% of the maximum number of buffers allowed, then
// writeback will be started. DEFAULT_FLUSH_THRESHOLD_PERCENT is the default value of X.
#define DEFAULT_FLUSH_THRESHOLD_PERCENT           30

// How many times the page replacement algorithm tries to find an eligible page before giving up
#define PAGE_REPL_NUM_TRIES                       3

// Any values of this size or less will be directly stored in btree leaf nodes.
// Values greater than this size will be stored in overflow blocks.
#define MAX_IN_NODE_VALUE_SIZE                    250

// In addition to the value itself we could potentially store memcached flags
// and a CAS value in the value contents, so we reserve space for that.
#define MAX_TOTAL_NODE_CONTENTS_SIZE              (MAX_IN_NODE_VALUE_SIZE + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint32_t))

// memcached specifies the maximum value size to be 1MB
#define MAX_VALUE_SIZE                            (1024 * KILOBYTE)

// Perform allocator GC every N milliseconds (the resolution is limited to TIMER_TICKS_IN_MS)
#define ALLOC_GC_INTERVAL_MS                      3000

//filenames for the database
#define DATA_DIRECTORY                            "db_data"

#define DATA_FNAME_BASE                           "data.file"

// We assume there will never be more than this many blocks. The value is computed by dividing
// 1 TB by the size of a block.
#define MAX_BLOCK_ID                              (TERABYTE / BTREE_BLOCK_SIZE)

// We assume that there will never be more than this many blocks held in memory by the cache at
// any one time. The value is computed by dividing 50 GB by the size of a block.
#define MAX_BLOCKS_IN_MEMORY                      (50 * GIGABYTE / BTREE_BLOCK_SIZE)

// This special block ID indicates the superblock. It doesn't really belong here because it's more
// of a magic constant than a tunable parameter.
#define SUPERBLOCK_ID                             0

// Every time the data file gets full, grow it by this many extents
#define FILE_GROWTH_RATE_IN_EXTENTS               5

#endif // __CONFIG_ARGS_H__

