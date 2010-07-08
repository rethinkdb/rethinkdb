# Default DEBUG variable to "on".  This should be changed on a release branch.
DEBUG ?= 1

# Define configuration variables
CXX=g++
LDFLAGS=-lrt -laio
CXXFLAGS=-Iinclude -Wall -Wextra -Wformat=2 -Wno-unused-parameter -Werror

DEP_DIR=dep
OBJ_DIR=obj
SERVER_EXEC_NAME=rethinkdb
TAGS=.tags

# Configure debug vs. release
ifeq (${DEBUG},1)
CXXFLAGS:=$(CXXFLAGS) -g -DDELETE_DEBUG
else
CXXFLAGS:=$(CXXFLAGS) -O3 -DNDEBUG
endif

# Build it in the correct directory
BUILD_DIR:=build
ifeq (${DEBUG},1)
BUILD_DIR:=$(BUILD_DIR)/debug
else
BUILD_DIR:=$(BUILD_DIR)/release
endif

START_DB_NAME=start_rethinkdb

ifdef VALGRIND
CXXFLAGS += -DVALGRIND
endif

# Define our objects
OBJS =  main.o \
	linux_io.o \
	event_queue.o \
	utils.o \
	worker_pool.o \
	server.o \
	cmd_args.o \
	message_hub.o \
	rwi_lock.o \
	cpu_context.o

DEPS:=$(OBJS:%.o=$(DEP_DIR)/%.d)
OBJS:=$(OBJS:%.o=$(OBJ_DIR)/%.o)

# High level build targets
$(SERVER_EXEC_NAME): $(OBJS)
	$(quiet_ld) $(CXX) $(LDFLAGS) $(OBJS) -o $(SERVER_EXEC_NAME)
	-mkdir -p $(BUILD_DIR)
	mv $(SERVER_EXEC_NAME) $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	cp ../scripts/$(START_DB_NAME) $(BUILD_DIR)/$(START_DB_NAME)
tags:
	@ctags -R -f $(TAGS)

style:
	@find . -name \*.cc -o -name \*.hpp | xargs ../scripts/cpplint --verbose 2 --filter=-whitespace/end_of_line,-whitespace/parens,-whitespace/line_length,-readability/casting,-whitespace/braces,-readability/todo,-legal/copyright,-whitespace/comments,-build/include,-whitespace/labels,-runtime/int,-runtime/printf 2>&1 | grep -v Done\ processing

clean:
	$(call quiet_rm, *~) find -name '*~' -exec rm {} \;
	$(call quiet_rm, $(BUILD_DIR)/$(SERVER_EXEC_NAME)) rm -f $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(call quiet_rm, $(BUILD_DIR)/$(START_DB_NAME)) rm -f $(BUILD_DIR)/$(START_DB_NAME)
	$(call quiet_rm, DEPS) rm -f $(DEPS)
	$(call quiet_rm, OBJS) rm -f $(OBJS)
	$(call quiet_rm, $(TAGS)) rm -f $(TAGS)

# Compiling files
$(OBJ_DIR)/%.o: %.cc Makefile
	$(quiet_cc) $(CXX) $(CXXFLAGS) -c -o $@ $<

# Build and include header file dependencies
$(DEP_DIR)/%.d: %.cc
	@$(CXX) $(CXXFLAGS) -M $< > $@.$$$$;                   \
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
