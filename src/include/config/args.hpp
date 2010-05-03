
#ifndef __CONFIG_ARGS_H__
#define __CONFIG_ARGS_H__

/**
 * Basic configuration parameters.
 * TODO: Many of these should be runtime switches.
 */
// Ticks (in seconds) the internal timed tasks are performed at
#define TIMER_TICKS_IN_SECS                       1

// Max concurrent IO requests per event queue
#define MAX_CONCURRENT_IO_REQUESTS                300

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

// Perform allocator GC every N ticks (where a tick is TIMER_TICKS_IN_SECS seconds)
#define ALLOC_GC_IN_TICKS                         3

// Size of the buffer used to perform IO operations (in bytes).
#define IO_BUFFER_SIZE                            4096

// Size of each btree node (in bytes)
#define BTREE_BLOCK_SIZE                          1024

// Max size of database file name
#define MAX_DB_FILE_NAME                          1024

// Ratio of free ram to use for the cache by default
#define DEFAULT_MAX_CACHE_RATIO                   0.8f

#endif // __CONFIG_ARGS_H__

