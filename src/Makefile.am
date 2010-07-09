# what flags do you want to pass to the C compiler and linker?
CXX:=g++
CXXFLAGS:=$(CXXFLAGS) -I. -Wall -Wextra -Wformat=2 -Wno-unused-parameter -Werror
LDFLAGS:=-lrt -laio

START_DB_NAME=start_rethinkdb
VPATH=./arch:./config:./concurrency
DEP_DIR=dep
OBJ_DIR=obj

# Configure debug vs. release
# and build in the correct directory accordingly
if CHK_NODEBUG
CXXFLAGS:=$(CXXFLAGS) -O3 -DNDEBUG
BUILD_DIR:=build/release
else
CXXFLAGS:=$(CXXFLAGS) -g
BUILD_DIR:=build/debug
endif

if CHK_VALGRIND
CXXFLAGS:=$(CXXFLAGS) -DVALGRIND
endif

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = rethinkdb
rethinkdb_SOURCES = main.cc arch/linux_io.cc event_queue.cc utils.cc worker_pool.cc server.cc config/cmd_args.cc message_hub.cc concurrency/rwi_lock.cc cpu_context.cc

DEPS:=$(rethinkdb_SOURCES:%.cc=$(DEP_DIR)/%.d)
OBJS:=$(rethinkdb_SOURCES:%.cc=$(OBJ_DIR)/%.o)


style:
	@find . -name \*.cc -o -name \*.hpp | xargs ../scripts/cpplint --verbose 2 --filter=-whitespace/end_of_line,-whitespace/parens,-whitespace/line_length,-readability/casting,-whitespace/braces,-readability/todo,-legal/copyright,-whitespace/comments,-build/include,-whitespace/labels,-runtime/int,-runtime/printf 2>&1 | grep -v Done\ processing


install: all-am
	@$(MAKE)
	mv *.o $(OBJ_DIR)
	mkdir -p $(BUILD_DIR)
	cp $(PACKAGE) $(BUILD_DIR)
	cp ../scripts/$(START_DB_NAME) $(BUILD_DIR)

clean:
    rm *.o rethinkdb
	rm -r $(BUILD_DIR)
	rm -f $(OBJS)

# Build and include header file dependencies
#$(DEP_DIR)/%.d: %.cc
#	@$(CXX) $(CXXFLAGS) -M $< > $@.$$$$;                   \
#	sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@;    \
#	rm -f $@.$$$$
#-include $(DEPS)
