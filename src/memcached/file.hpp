#ifndef __MEMCACHED_FILE_HPP__
#define __MEMCACHED_FILE_HPP__

#include <string>
#include "store.hpp"
#include "concurrency/fifo_checker.hpp"

/* `import_memcache()` opens the file specified by its first parameter and reads
memcache commands from it, sending the commands to the given `set_store_interface_t`.
It stops when it reaches the end of the file or when SIGINT is delivered to the
server.

The main use of `import_memcache()` is to implement the `rethinkdb import`
subcommand.*/

void import_memcache(std::string, set_store_interface_t *set_store, order_source_t *order_source);

#endif /* __MEMCACHED_FILE_HPP__ */
