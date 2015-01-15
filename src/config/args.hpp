// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONFIG_ARGS_HPP_
#define CONFIG_ARGS_HPP_

#define KILOBYTE 1024LL
#define MEGABYTE (KILOBYTE * 1024LL)
#define GIGABYTE (MEGABYTE * 1024LL)
#define TERABYTE (GIGABYTE * 1024LL)

#define THOUSAND 1000LL
#define MILLION (THOUSAND * THOUSAND)
#define BILLION (THOUSAND * MILLION)

/*!
 * Version strings
 */

#define SOFTWARE_NAME_STRING "RethinkDB"

// The SERIALIZER_VERSION_STRING might remain unchanged for a while -- individual
// metablocks now have a disk_format_version field that can be incremented for
// on-the-fly version updating.
#define SERIALIZER_VERSION_STRING "1.13"

// See also CLUSTER_VERSION_STRING and cluster_version_t.

/**
 * Basic configuration parameters.
 */

// The number of hash-based CPU shards per table.
// This "must" be hard-coded because a cluster cannot run with
// differing cpu sharding factors.
#define CPU_SHARDING_FACTOR                       8

// Defines the maximum size of the batch of IO events to process on
// each loop iteration. A larger number will increase throughput but
// decrease concurrency
#define MAX_IO_EVENT_PROCESSING_BATCH_SIZE        50

// The io batch factor ensures a minimum number of i/o operations
// which are picked from any specific i/o account consecutively.
// A higher value might be advantageous for throughput if seek times
// matter on the underlying i/o system. A low value improves latency.
// The advantage only holds as long as each account has a tendentially
// sequential set of i/o operations though. If access patterns are random
// for all accounts, a low io batch factor does just as well (as bad) when it comes
// to the number of random seeks, but might still provide a lower latency
// than a high batch factor. Our serializer now groups data writes into
// a single large write operation by itself, making a high value here less
// useful.
#define DEFAULT_IO_BATCH_FACTOR                   1

// I/O priority of index writes in the log serializer
#define INDEX_WRITE_IO_PRIORITY                   128

// Each cache uses its own I/O account for reads.
// Writes use the default I/O account, which in practice is going to be overridden
// by a common account in the merger_serializer_t.
//
// By adjusting the priority of the read account, relative to
// MERGER_BLOCK_WRITE_IO_PRIORITY below, reads can be prioritized over writes
// or the other way around.
//
// In practice we want to prioritize reads, so reads can pass ahead of
// flushes.  The rationale behind this is that reads are almost always blocking
// operations.  Writes, on the other hand, can be non-blocking (from the user's
// perspective) if they are soft-durability or noreply writes.
#define CACHE_READS_IO_PRIORITY                   (512 / CPU_SHARDING_FACTOR)

// The cache priority to use for secondary index post construction
// 100 = same priority as all other read operations in the cache together.
// 0 = minimal priority
#define SINDEX_POST_CONSTRUCTION_CACHE_PRIORITY   5

// Size of the buffer used to perform IO operations (in bytes).
#define IO_BUFFER_SIZE                            (4 * KILOBYTE)

// Size of the device block size (in bytes)
#define DEVICE_BLOCK_SIZE                         512

// Size of the metablock (in bytes)
#define METABLOCK_SIZE                            (4 * KILOBYTE)

// Size of each btree node (in bytes) on disk
#define DEFAULT_BTREE_BLOCK_SIZE                  (4 * KILOBYTE)

// Size of each extent (in bytes)
// This should not be too small, or garbage collection will become
// inefficient (especially on rotational drives).
#define DEFAULT_EXTENT_SIZE                       (2 * MEGABYTE)

// Ratio of free ram to use for the cache by default
#define DEFAULT_MAX_CACHE_RATIO                   2

// The maximum number of concurrently active
// index writes per merger serializer.
// The smaller the number, the more effective
// the merger serializer is in merging index writes
// together. This is favorable especially on rotational drives.
// There is a theoretic chance of increased latencies on SSDs for
// small values of this variable.
#define MERGER_SERIALIZER_MAX_ACTIVE_WRITES       1

// I/O priority of block writes in the merger_serializer_t
#define MERGER_BLOCK_WRITE_IO_PRIORITY            64

// Maximum number of threads we support
// TODO: make this dynamic where possible
#define MAX_THREADS                               128

// Ticks (in milliseconds) the internal timed tasks are performed at
#define TIMER_TICKS_IN_MS                         5

// How many milliseconds to allow changes to sit in memory before flushing to disk
#define DEFAULT_FLUSH_TIMER_MS                    1000

// If non-null disk_ack_signals are present, concurrent flushing can be used to reduce the
// latency of each single flush. max_concurrent_flushes controls how many flushes can be active
// on a specific slice at any given time.
#define DEFAULT_MAX_CONCURRENT_FLUSHES            1

// How many times the page replacement algorithm tries to find an eligible page before giving up.
// Note that (MAX_UNSAVED_DATA_LIMIT_FRACTION ** PAGE_REPL_NUM_TRIES) is the probability that the
// page replacement algorithm will succeed on a given try, and if that probability is less than 1/2
// then the page replacement algorithm will on average be unable to evict pages from the cache.
#define PAGE_REPL_NUM_TRIES                       10

// How large can the key be, in bytes?  This value needs to fit in a byte.
#define MAX_KEY_SIZE                              250

// Special block IDs.  These don't really belong here because they're
// more magic constants than tunable parameters.

// The btree superblock, which has a reference to the root node block
// id.
#define SUPERBLOCK_ID                             0

// If the size of the LBA on a given disk exceeds LBA_MIN_SIZE_FOR_GC, then the fraction of the
// entries that are live and not garbage should be at least LBA_MIN_UNGARBAGE_FRACTION.
#define LBA_MIN_SIZE_FOR_GC                       (MEGABYTE * 1)
#define LBA_MIN_UNGARBAGE_FRACTION                0.5

// I/O priority for LBA garbage collection
#define LBA_GC_IO_PRIORITY                        8

// How many block ids should the LBA garbage collector rewrite before yielding?
#define LBA_GC_BATCH_SIZE                         (1024 * 8)

// How many LBA structures to have for each file
#define LBA_SHARD_FACTOR                          4

// How much space to reserve in the metablock to store inline LBA entries
// Make sure that it fits into METABLOCK_SIZE, including all other meta data
// TODO (daniel): Tune
#define LBA_INLINE_SIZE                           (METABLOCK_SIZE - 512)

// How many bytes of buffering space we can use per disk when reading the LBA. If it's set
// too high, then RethinkDB will eat a lot of memory at startup. This is bad because tcmalloc
// doesn't return memory to the OS. If it's set too low, startup will take a longer time.
#define LBA_READ_BUFFER_SIZE                      (128 * MEGABYTE)

// After the LBA has been read, we reconstruct the in-memory LBA index.
// For huge tables, this can take some considerable CPU time. We break the reconstruction
// up into smaller batches, each batch reconstructing up to `LBA_RECONSTRUCTION_BATCH_SIZE`
// block infos.
#define LBA_RECONSTRUCTION_BATCH_SIZE             1024

#define COROUTINE_STACK_SIZE                      131072

// How many unused coroutine stacks to keep around (maximally), before they are
// freed. This value is per thread.
#define COROUTINE_FREE_LIST_SIZE                  64

#define MAX_COROS_PER_THREAD                      10000


/**
 * Message scheduler configuration
 */

// Each message on the message hup can have a priority between
// MESSAGE_SCHEDULER_MIN_PRIORITY and MESSAGE_SCHEDULER_MAX_PRIORITY (both inclusive).
#define MESSAGE_SCHEDULER_MIN_PRIORITY          (-2)
#define MESSAGE_SCHEDULER_MAX_PRIORITY          2

// If no priority is specified, messages will get MESSAGE_SCHEDULER_DEFAULT_PRIORITY.
#define MESSAGE_SCHEDULER_DEFAULT_PRIORITY      0

// Ordered messages cannot currently have different priorities, because that would mean
// that ordered messages of a high priority could bypass those of a lower priority.
// (our current implementation does not support re-ordering messages within a given
// priority)
// MESSAGE_SCHEDULER_ORDERED_PRIORITY is the effective priority at which ordered
// messages are scheduled.
#define MESSAGE_SCHEDULER_ORDERED_PRIORITY      0

// MESSAGE_SCHEDULER_GRANULARITY specifies how many messages (of
// MESSAGE_SCHEDULER_MAX_PRIORITY) the message scheduler processes before it
// can take in new incoming messages. A smaller value means that high-priority
// messages can bypass lower-priority ones faster, but decreases the efficiency
// of the message hub.
// MESSAGE_SCHEDULER_GRANULARITY should be least
// 2^(MESSAGE_SCHEDULER_MAX_PRIORITY - MESSAGE_SCHEDULER_MIN_PRIORITY + 1)
#define MESSAGE_SCHEDULER_GRANULARITY           32

// Priorities for specific tasks
#define CORO_PRIORITY_SINDEX_CONSTRUCTION       (-2)
#define CORO_PRIORITY_BACKFILL_SENDER           (-2)
#define CORO_PRIORITY_BACKFILL_RECEIVER         (-2)
#define CORO_PRIORITY_RESET_DATA                (-2)
#define CORO_PRIORITY_REACTOR                   (-1)
#define CORO_PRIORITY_DIRECTORY_CHANGES         (-2)
#define CORO_PRIORITY_LBA_GC                    (-2)


#endif  // CONFIG_ARGS_HPP_

