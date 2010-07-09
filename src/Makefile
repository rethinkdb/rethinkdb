# Default DEBUG variable to "on".  This should be changed on a release branch.
DEBUG ?= 1

# Define configuration variables
CXX=g++
LDFLAGS=-lrt -laio
CXXFLAGS=-I. -Wall -Wextra -Wformat=2 -Wno-unused-parameter -Werror

DEP_DIR=dep
OBJ_DIR=obj
SERVER_EXEC_NAME=rethinkdb
BUILD_DIR=../build
TAGS=.tags

# Configure debug vs. release
ifeq (${DEBUG},1)
SERVER_EXEC_NAME:=$(BUILD_DIR)/debug/$(SERVER_EXEC_NAME)
CXXFLAGS:=$(CXXFLAGS) -g
else
SERVER_EXEC_NAME:=$(BUILD_DIR)/release/$(SERVER_EXEC_NAME)
CXXFLAGS:=$(CXXFLAGS) -O3 -DNDEBUG
endif

ifdef VALGRIND
CXXFLAGS += -DVALGRIND
endif

# Define our objects
OBJS =  arch/linux_io.o \
	concurrency/rwi_lock.o \
	config/cmd_args.o \
	cpu_context.o \
	event_queue.o \
	main.o \
	message_hub.o \
	server.o \
	utils.o \
	worker_pool.o

DEPS:=$(OBJS:%.o=$(DEP_DIR)/%.d)
OBJS:=$(OBJS:%.o=$(OBJ_DIR)/%.o)

# High level build targets
$(SERVER_EXEC_NAME): $(OBJS)
	@mkdir -p $(dir $(SERVER_EXEC_NAME))
	$(quiet_ld) $(CXX) $(LDFLAGS) $(OBJS) -o $(SERVER_EXEC_NAME)

tags:
	@ctags -R -f $(TAGS)

style:
	@find . -name \*.cc -o -name \*.hpp | xargs ../scripts/cpplint --verbose 2 --filter=-whitespace/end_of_line,-whitespace/parens,-whitespace/line_length,-readability/casting,-whitespace/braces,-readability/todo,-legal/copyright,-whitespace/comments,-build/include,-whitespace/labels,-runtime/int,-runtime/printf 2>&1 | grep -v Done\ processing

clean:
	$(call quiet_rm, *~) find -name '*~' -exec rm {} \;
	$(call quiet_rm, $(SERVER_EXEC_NAME)) rm -f $(SERVER_EXEC_NAME)
	$(call quiet_rm, DEPS) rm -f $(DEPS)
	$(call quiet_rm, OBJS) rm -f $(OBJS)
	$(call quiet_rm, $(TAGS)) rm -f $(TAGS)

# Compiling files
$(OBJ_DIR)/%.o: %.cc Makefile
	@mkdir -p $(dir $@)
	$(quiet_cc) $(CXX) $(CXXFLAGS) -c -o $@ $<

# Build and include header file dependencies
$(DEP_DIR)/%.d: %.cc
	@mkdir -p $(dir $@);                                              \
	$(CXX) $(CXXFLAGS) -M $< > $@.$$$$;                               \
	sed 's,\($*\)\.o[ :]*,$(OBJ_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@;    \
	rm -f $@.$$$$

-include $(DEPS)

# Define pretty printing functions
ifeq (${VERBOSE},1)
	quiet_cc =
	quiet_ld =
	quiet_rm =
else
	quiet_cc = @echo "  CC " $< ;
	quiet_ld = @echo "  LD " $@ ;
	quiet_rm = @echo "  RM " $(1) ;
endif
