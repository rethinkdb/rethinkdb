# Define configuration variables
CXX=g++
LDFLAGS=-lrt -laio
CXXFLAGS=-Iinclude

DEP_DIR=dep
OBJ_DIR=obj
SERVER_EXEC_NAME=rethink

# Configure debug vs. release
ifeq (${DEBUG},1)
CXXFLAGS:=$(CXXFLAGS) -g
else
CXXFLAGS:=$(CXXFLAGS) -O3 -DNDEBUG
endif

# Define our objects
OBJS =  main.o \
	async_io.o \
	event_queue.o \
	utils.o \
	worker_pool.o \
	tty.o \
	server.o \
	fsm.o \
	fsm_ops.o \
	fsm_msg.o

DEPS:=$(OBJS:%.o=$(DEP_DIR)/%.d)
OBJS:=$(OBJS:%.o=$(OBJ_DIR)/%.o)

# High level build targets
$(SERVER_EXEC_NAME): $(OBJS)
	$(quiet_ld) $(CXX) $(LDFLAGS) $(OBJS) -o $(SERVER_EXEC_NAME)

clean:
	$(call quiet_rm, *~) find -name '*~' -exec rm {} \;
	$(call quiet_rm, $(SERVER_EXEC_NAME)) rm -f $(SERVER_EXEC_NAME)
	$(call quiet_rm, DEPS) rm -f $(DEPS)
	$(call quiet_rm, OBJS) rm -f $(OBJS)

# Compiling files
$(OBJ_DIR)/%.o: %.cc Makefile
	$(quiet_cc) $(CXX) $(CXXFLAGS) -c -o $@ $<

# Build and include header file dependencies
$(DEP_DIR)/%.d: %.cc
	@$(CXX) $(CXXFLAGS) -M $< > $@.$$$$;                   \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;    \
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

