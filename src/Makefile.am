# what flags you want to pass to the C compiler & linker
CXXFLAGS=-I. -Iarch -Iconcurrency -Iconfig -Wall -Wextra -Wformat=2 -Wno-unused-parameter -Werror
LDFLAGS=-lrt -laio
VPATH=./arch:./config:./concurrency
# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = rethinkdb
rethinkdb_SOURCES = main.cc arch/linux_io.cc event_queue.cc utils.cc worker_pool.cc server.cc config/cmd_args.cc message_hub.cc concurrency/rwi_lock.cc cpu_context.cc