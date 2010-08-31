##### Configuring stuff

SERVER_EXEC_NAME:=rethinkdb
SCRIPTS_DIR:=../scripts
START_DB_NAME:=start_rethinkdb
GDB_FUNCTIONS_NAME:=rethinkdb-gdb.py
TAGS:=.tags
RUN_PORT?=11211
RUN_FLAGS?=
VALGRIND_FLAGS?=--leak-check=full --db-attach=yes
DEBUG?=1
VALGRIND?=0
VERBOSE?=0

# Choose our directories
# This makefile is meant to be run from the 'src' directory.
SOURCE_DIR:=.
BUILD_ROOT_DIR:=../build
ifeq (${DEBUG},1)
BUILD_DIR:=$(BUILD_ROOT_DIR)/debug
else
BUILD_DIR:=$(BUILD_ROOT_DIR)/release
endif
ifeq ($(VALGRIND),1)
BUILD_DIR:=$(BUILD_DIR)-valgrind
endif
DEP_DIR:=$(BUILD_DIR)/dep
OBJ_DIR:=$(BUILD_DIR)/obj

# Define configuration variables
CXX:=g++
LDFLAGS:=-lrt -laio
CXXFLAGS:=-I$(SOURCE_DIR) -Wall -Wextra -Wformat=2 -Wno-unused-parameter -Werror

# Configure debug vs. release
ifeq ($(DEBUG),1)
CXXFLAGS+=-g
# -rdynamic is necessary so that backtrace_symbols() works properly
LDFLAGS+=-rdynamic
else
CXXFLAGS+=-O3 -DNDEBUG
endif
ifdef VALGRIND
CXXFLAGS+=-DVALGRIND
endif

ifeq ($(BTREE_DEBUG),1)
CXXFLAGS+=-DBTREE_DEBUG
endif

ifeq ($(SERIALIZER_DEBUG),1)
CXXFLAGS:=$(CXXFLAGS) -DSERIALIZER_MARKERS
endif

ifeq ($(MEMCACHED_STRICT), 1)
CXXFLAGS+=-DMEMCACHED_STRICT
endif

# Should makefile be noisy?
ifeq ($(VERBOSE),1)
QUIET:=
else
QUIET:=@
endif



##### Finding what to build

SOURCES:=$(shell find $(SOURCE_DIR) -name \*.cc)
NAMES:=$(patsubst $(SOURCE_DIR)/%.cc,%,$(SOURCES))
DEPS:=$(patsubst %,$(DEP_DIR)/%.d,$(NAMES))
OBJS:=$(patsubst %,$(OBJ_DIR)/%.o,$(NAMES))



##### Build targets

# High level build targets

$(BUILD_DIR)/$(SERVER_EXEC_NAME): $(OBJS) $(BUILD_DIR)
ifeq ($(VERBOSE),0)
	@echo "    LD $@"
endif
	$(QUIET) $(CXX) $(LDFLAGS) $(OBJS) -o $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(QUIET) cp $(SCRIPTS_DIR)/$(START_DB_NAME) $(BUILD_DIR)/$(START_DB_NAME)
	$(QUIET) cp $(SCRIPTS_DIR)/$(GDB_FUNCTIONS_NAME) $(BUILD_DIR)/$(GDB_FUNCTIONS_NAME)
# The 'run' and 'gdb' targets build the server, automatically remove old data files, and then start
# it up. The difference is that the 'gdb' target starts it under a debugger.

run: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(QUIET) rm -rf db_data
	$(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) $(RUN_FLAGS)

gdb: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
	$(QUIET) rm -rf db_data
	gdb --args $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) $(RUN_FLAGS)

valgrind: $(BUILD_DIR)/$(SERVER_EXEC_NAME)
ifeq ($(VALGRIND),0)
	$(error Using the 'valgrind' target doesn't automatically set VALGRIND to 1. Try again with \
'make valgrind VALGRIND=1')
endif
	$(QUIET) rm -rf db_data
	valgrind $(VALGRIND_FLAGS) $(BUILD_DIR)/$(SERVER_EXEC_NAME) -p $(RUN_PORT) $(RUN_FLAGS)

tags:
	$(QUIET) ctags -R -f $(TAGS) --langmap="c++:.cc.tcc.hpp"

style:
	$(QUIET) find . -name \*.cc -o -name \*.hpp | xargs ../scripts/cpplint --verbose 2 --filter=-whitespace/end_of_line,-whitespace/parens,-whitespace/line_length,-readability/casting,-whitespace/braces,-readability/todo,-legal/copyright,-whitespace/comments,-build/include,-whitespace/labels,-runtime/int,-runtime/printf 2>&1 | grep -v Done\ processing

clean:
ifeq ($(VERBOSE),0)
	@echo "    RM *~"
	@echo "    RM -r $(BUILD_ROOT_DIR)"
	@echo "    RM $(TAGS)"
endif
	$(QUIET) find -name '*~' -exec rm {} \;
	$(QUIET) rm -r $(BUILD_ROOT_DIR)
	$(QUIET) rm -f $(TAGS)

# Directories
$(BUILD_DIR):
	$(QUIET) mkdir -p $(BUILD_DIR)

# Object files
$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cc Makefile
	$(QUIET) mkdir -p $(dir $@)
ifeq ($(VERBOSE),0)
	@echo "    CC $< -o $@"
endif
	$(QUIET) $(CXX) $(CXXFLAGS) -c -o $@ $<

# Dependencies
$(DEP_DIR)/%.d: $(SOURCE_DIR)/%.cc
	$(QUIET) mkdir -p $(dir $@)
	$(QUIET) $(CXX) $(CXXFLAGS) -M $< > $@.$$$$;				   \
	sed 's,$(notdir $*)\.o[ :]*,$(OBJ_DIR)/$*.o $@ : ,g' < $@.$$$$ > $@;	  \
	rm -f $@.$$$$
# Include the dependencies into the makefile so that they take effect
-include $(DEPS)
