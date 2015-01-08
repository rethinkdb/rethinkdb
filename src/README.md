## Source code description

A brief description of src/ and subdirectories.

### src/
Main function, just calls to command line parser from administration/main/,
error and backtrace definitions, main interfaces (abstract classes),
home thread mixin for objects that can only be used on a single thread, math utils, etc..



### arch/
Architecture-dependent code, spinlocks, timers, barriers, files, etc..

#### arch/io
IO read/write worker threads, condition variables and mutexes, tcp/udp sockets.

##### arch/io/disk
IO operation scheduler, IO event listening, IO throttling, IO statistics.

##### arch/io/timer
One shot timers with callbacks for each platform.

#### arch/runtime
Coroutines/fibers, message hub/queue (one per thread with MAX_THREADS queues), etc..
Every function runs inside coroutines inside threads (uses msg hub to store next coros).

##### arch/runtime/event_queue
Arch-dependent event queue with a callback on_event().

##### arch/runtime/system_event
Arch-dependent condition variable using pipes/eventfd.



### btree/
B-tree operations on top of buffer caches, btree slice, secondary indexes, etc..



### buffer_cache/
Typedefs for main buffer classes, buf_lock_t, cache_t, transaction_t, etc..

#### buffer_cache/alt
Memory page/block eviction, replacement and flush policies, txn and snapshots, etc..



### clustering/
Rethink cluster logic, node roles, blueprint implementation, command-line / web backends, etc..

#### clustering/administration
Implementation of reactor driver/manager, loggers and metadata definitions for namespace,
database, server, etc..

##### clustering/administration/cli
Helper functions to parse commands from command-line interface.

##### clustering/administration/http
HTTP user applications for system log, backfilling progress, etc.

##### clustering/administration/issues
Cluster system issues/problems that need to be resolved.

##### clustering/administration/main
Rethink server main options and startup functions.

#### clustering/generic
Throttling mechanism for query processing in a cluster node, uses clustered mailboxes.

#### clustering/immediate_consistency
Data and shard consistency mechanisms using clustered mailboxes known as business cards.

##### clustering/immediate_consistency/branch
Read/Write queries are sent to the primary server of a shard (master_t).
The master_t forwards them to the brodcaster_t (primary replica) of that shard.
The broadcaster_t sorts and distributes them to one or more listener_t,

A listener_t is the cluster-facing interface of a replica for a single shard.
A listener_t performs read/writes to the B-tree.
A secondary replica is essentially just a listener_t.

History of table data regions (shards) is identified by a branch ID + timestamp.
A branch is the DB state when a broadcaster_t was created + sequence of writes.

The replier_t basically is there to wait for the backfilling to complete (from sec to pri),
and only then it will tell the broadcaster_t that the listener_t can now also
receive (up-to-date) read requests.

##### clustering/immediate_consistency/query
Query processing for each shard.
A primary server of a shard has a master_t receiving queries from other nodes.

#### clustering/reactor
Cluster node functions to match the blueprint. A node role can be primary, secondary or nothing/sleep.



### concurrency/
Architecture-independent concurrent structures used by threads and coroutines.

#### concurrency/queue
Concurrent queues.



### containers/
Auth key, blob, bitset, buffer group, list, queue, two-level array, etc. Some are serializable.

#### containers/archive
RDB custom serialization format implementation (e.g. used for cluster node communication).


### extproc/
External process (fork) pool, workers/jobs and javascript job that uses V8 (js engine).



### http/
HTTP server/parser and request router.

#### http/json
Custom cJSON wrapper with iterators, etc.



### memcached/
Memcached protocol server, parser, etc.. (abandoned and replaced with reql)

#### memcached/memcached_btree
Memcached wrapper for btree operations.



### mock/
Dummy protocol and serializer implementation for testing.



### parsing/
Line parser for TCP sockets.



### perfmon/
Performance monitor, they're used across the code base for stats.



### protob/
Server that parses protobuf requests/responses (subclass of http_app_t).



### rdb_protocol/
Sharding, node backfill protocol (uses RDB SERIALIZABLE format) and query parser/protocol (uses protobuf).

#### rdb_protocol/terms
Serialization and evaluation of protocol terms, functions, expressions, etc.



### region/
Implementation of cluster regions for key/data sharding.



### rpc/
Cluster/Inter-node communication mechanisms.

#### rpc/connectivity
Message handler and service interfaces for implementing small clustered/inter-node apps.

#### rpc/directory
A directory is a clustered app that stores metadata for every node, where node/peer ID is the key.

#### rpc/mailbox
Mailbox impl on top of message services, a mailbox belongs to a thread running in a node.

#### rpc/semilattice
A semilattice is a sub-region of metadata.
A semilattice manager is a clustered app that keeps metadata in sync (through joins) between nodes.

##### rpc/semilattice/joins
Semilattice joinable types (including virtual clocks).

##### rpc/semilattice/view
Semilattice observers (can also write through a join).



### serializer/
IO Serializers. Multiplexer/translator, merger (for bulk index ops) and log.

#### serializer/log
DB file extent/metablock/block managers and index journal/log.

##### serializer/log/lba
Structure of DB files (LBA means Logical Block Addressing).

