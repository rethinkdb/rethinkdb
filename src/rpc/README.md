## RethinkDB RPC concepts

### Connections

Every server in the RethinkDB cluster maintains a TCP connection to every other server. The `connectivity_cluster_t` (defined in `rpc/connectivity/cluster.hpp`) is responsible for managing these connections. The `connectivity_cluster_t` exposes a low-level interface for sending untyped binary string messages between servers. The messages are guaranteed to be delivered uncorrupted and exactly once, unless the connection is lost. A common pattern is to send a message and wait for a reply, but abort the attempt if the connection is detected to have been lost; `disconnect_watcher_t` can be used to receive a notification if the connection is lost. Messages are not guaranteed to be delivered in order.

### Mailboxes

Most code doesn't use the `connectivity_cluster_t` directly; instead, it works with "mailboxes". A mailbox (see `rpc/mailbox/`) is a object that is created dynamically on a specific server to receive messages of a specific type. Every mailbox has an "address", which can be serialized and sent to other servers in the cluster; the other servers can use the address to send messages to the mailbox. The mailbox code uses C++ templates to make sure that the right type of message is sent, and to automatically serialize and deserialize it. RethinkDB uses a custom system based on C++ templates for serialization and deserialization; see `containers/archive/`.

If some code tries to send a message to a mailbox that doesn't exist anymore, or exists on a disconnected server, the message will be silently dropped. To detect the former situation, the sender should find out that the mailbox no longer exists, usually by the same mechanism that it found out that the mailbox existed in the first place. To detect the latter situation, the sender should use a `disconnect_watcher_t`.

### Directory

Mailboxes have a chicken-and-egg problem: in order to send a message to a mailbox, one must have its address, but the only way to find out the address is to receive it in a message from a mailbox. This is resolved by using the "directory". The directory (see `rpc/directory/`) is a collection of serializable objects that each server broadcasts to the other servers it is connected to. The directory contains basic metadata about the server such as its name, as well as the addresses of mailboxes that can be used to execute users' queries, change table configurations, check the status of the server, and so on. The objects in the directory are often called "business cards", since they contain mailbox addresses.

The directory is broken into several parts. One part of the directory consists of a single value per server; the type `directory_write_manager_t<V>` is responsible for sending this value over the network to a `directory_read_manager_t<V>` on each other server. Other parts of the directory consist of many individual values associated with specific keys. This saves network traffic when only one part needs to be updated. The types associated with these are called `directory_map_write_manager_t<K, V>` and `directory_map_read_manager_t<K, V>`. The specific directory read/write managers are instantiated in `do_serve()` in `clustering/administration/main/serve.cc`.

Some parts of the directory are constant, but others change dynamically. For example, if a server becomes a replica for a new table, it will put new business cards in its directory with the addresses of mailboxes that process queries for that table.

### Semilattices

The databases and auth key are stored in the "semilattices", an eventually consistent metadata storage system. Whenever two servers connect, they exchange their semilattices, to ensure that each one gets all the latest updates. Conflicts are resolved using timestamps. `semilattice_manager_t<T>` in `rpc/semilattice/` is responsible for this process.

In earlier versions of RethinkDB (pre-2.1) table configuration and server configuration were also stored in the semilattices.
