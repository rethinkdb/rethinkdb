// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef MEMCACHED_FILE_HPP_
#define MEMCACHED_FILE_HPP_

class signal_t;
class set_store_interface_t;


/* `import_memcache()` opens the file specified by its first parameter and reads
memcache commands from it, sending the commands to the given `set_store_interface_t`.
It stops when it reaches the end of the file or when the `interrupt` cond that you
pass is pulsed.

The main use of `import_memcache()` is to implement the `rethinkdb import`
subcommand.*/

void import_memcache(const char *filename, set_store_interface_t *set_store, int n_slices, signal_t *interrupt);

#endif /* MEMCACHED_FILE_HPP_ */
