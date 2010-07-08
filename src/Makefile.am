# what flags you want to pass to the C compiler & linker
CFLAGS=-Iinclude -Wall -Wextra -Wformat=2 -Wno-unused-parameter -Werror
LDFLAGS=-lrt -laio

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = rethinkdb
rethinkdb_SOURCES = main.o linux_io.o event_queue.o utils.o worker_pool.o server.o cmd_args.o message_hub.o rwi_lock.o cpu_context.o