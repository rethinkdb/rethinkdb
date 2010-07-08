# what flags you want to pass to the C compiler & linker
CXX:=g++
CXXFLAGS:=$(CXXFLAGS) -I. -Wall -Wextra -Wformat=2 -Wno-unused-parameter -Werror
LDFLAGS:=-lrt -laio

BUILD_DIR:=build
START_DB_NAME=start_rethinkdb

# Configure debug vs. release
# and build in the correct directory accordingly
if CHK_DEBUG
CXXFLAGS:=$(CXXFLAGS) -g
BUILD_DIR:=$(BUILD_DIR)/debug
else
CXXFLAGS:=$(CXXFLAGS) -O3 -DNDEBUG
BUILD_DIR:=$(BUILD_DIR)/release
endif

if CHK_VALGRIND
CXXFLAGS:=$(CXXFLAGS) -DVALGRIND
endif

VPATH=./arch:./config:./concurrency
# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = rethinkdb
rethinkdb_SOURCES = main.cc arch/linux_io.cc event_queue.cc utils.cc worker_pool.cc server.cc config/cmd_args.cc message_hub.cc concurrency/rwi_lock.cc cpu_context.cc

