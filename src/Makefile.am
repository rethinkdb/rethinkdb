# what flags you want to pass to the C compiler & linker
CXXFLAGS=-Iinclude -Wall -Wextra -Wformat=2 -Wno-unused-parameter -Werror
LDFLAGS=-lrt -laio

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = rethinkdb
rethinkdb_SOURCES = main.cc linux_io.cc event_queue.cc utils.cc worker_pool.cc server.cc cmd_args.cc message_hub.cc rwi_lock.cc cpu_context.cc