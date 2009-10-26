
#ifndef __CONFIG__H__
#define __CONFIG__H__

// Max concurrent IO requests per event queue
#define MAX_CONCURRENT_IO_REQUESTS                300

// A hint about the average number of concurrent network events
#define CONCURRENT_NETWORK_EVENTS_COUNT_HINT      300

// Defines the maximum size of the batch of IO events to process on
// each loop iteration. A larger number will increase throughput but
// decrease concurrency
#define MAX_IO_EVENT_PROCESSING_BATCH_SIZE        10

// Define the size of the allocator heap per worker in bytes
#define ALLOCATOR_WORKER_HEAP                     1024*1024*8

#endif // __CONFIG__H__

