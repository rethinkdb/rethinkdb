
#ifndef __CONFIG_ARGS_H__
#define __CONFIG_ARGS_H__

/**
 * Basic configuration parameters.
 * TODO: Many of these should be runtime switches.
 */
// Ticks (in milliseconds) the internal timed tasks are performed at
#define TIMER_TICKS_IN_MS                         3000

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

// Perform allocator GC every N milliseconds (the resolution is limited to TIMER_TICKS_IN_MS)
#define ALLOC_GC_IN_MS                            3000

// Size of the buffer used to perform IO operations (in bytes).
#define IO_BUFFER_SIZE                            4096

// Size of each btree node (in bytes)
#define BTREE_BLOCK_SIZE                          1024

// Max size of database file name
#define MAX_DB_FILE_NAME                          1024

// Ratio of free ram to use for the cache by default
#define DEFAULT_MAX_CACHE_RATIO                   0.8f

// Maximum number of operations packed into a single request
// TODO: make this dynamic and get rid of this parameter
#define MAX_OPS_IN_REQUEST                        16

// Maximum number of CPUs we support
// TODO: make this dynamic where possible
#define MAX_CPUS                                  16

// The number of ways we split a BTree (the most optimal is the number
// of cores, but we use a higher split factor to allow upgrading to
// more cores without migrating the database file).
// TODO: when we build a real serializer, we should make this option
// configurable on a per-database level (with a sensible default), and
// provide a migration tool in case the customers upgrade to more
// cores than was originally anticipated.
#define BTREE_SPLIT_FACTOR                        16

// Default port to listen on
#define DEFAULT_LISTEN_PORT                       8080

#endif // __CONFIG_ARGS_H__

