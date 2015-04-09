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
Memory page/block eviction, replacement and flush policies, txn and snapshots, etc..



### clustering/
Rethink cluster logic, node roles, blueprint implementation, command-line / web backends, etc..

#### clustering/administration
Implementation of "main"-like function for the server. Glue code to hold together all the
different components. Logger. Metadata definitions for table, database, server, etc...
Administrative tools like the system tables.

##### clustering/administration/http
HTTP user applications for system log, backfilling progress, etc.

##### clustering/administration/issues
Cluster system issues/problems that need to be resolved.

##### clustering/administration/main
Rethink server main options and startup functions.

#### clustering/generic
Throttling mechanism for query processing in a cluster node, uses clustered mailboxes.

#### clustering/immediate_consistency
Handles assigning timestamps to queries and enforcing consistency.

Every shard's primary replica has a `broadcaster_t`. Clients send read and write queries
to the `broadcaster_t` via the types in `clustering/query_routing`. The `broadcaster_t`
assigns them timestamps and distributes them to the `listener_t`s. Every primary and
secondary replica has a `listener_t`. The `listener_t` accepts queries from the
`broadcaster_t` and applies them to the storage engine.

When the `listener_t` is first created, it also takes care of backfilling data from
another replica and synchronizing the stream of writes from the `broadcaster_t` with the
backfill it got from the other replica. `replier_t` modifies the behavior of the
`listener_t` by allowing it to respond to reads as well as writes.

The branch history is a collection of metadata that's used to coordinate backfills. A
"branch" is a sequence of writes all timestamped in order by the same `broadcaster_t`.
Whenever the primary replica restarts or changes to another server, it creates a new
branch derived from the earlier branch. This allows the server to reason about divergent
data.

#### clustering/query_routing
Handles transferring queries from the parser to replicas. Also takes care of splitting
queries into parts for different shards.

#### clustering/table_manager
Uses Raft to coordinate creating/deleting/reconfiguring tables.



### concurrency/
Architecture-independent concurrent structures used by threads and coroutines.

#### concurrency/queue
Concurrent queues.



### containers/
Auth key, blob, buffer group, list, queue, two-level array, etc. Some are serializable.

#### containers/archive
RDB custom serialization format implementation (e.g. used for cluster node communication).


### extproc/
External process (fork) pool, workers/jobs and javascript job that uses V8 (js engine).



### http/
HTTP server/parser and request router.

#### http/json
Custom cJSON wrapper with iterators, etc.



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

