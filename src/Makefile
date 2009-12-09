# Define configuration variables
CXX=g++
LDFLAGS=-lrt -laio
CXXFLAGS=-Iinclude
SERVER_EXEC_NAME=rethink

DEP_DIR=dep

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
	server.o

# High level build targets
$(SERVER_EXEC_NAME): $(OBJS)
	$(quiet_ld)
	$(Q) $(CXX) $(LDFLAGS) $(OBJS) -o $(SERVER_EXEC_NAME)

clean:
	rm -f $(SERVER_EXEC_NAME) pool_test
	rm -rf *~ *.o
	rm -f $(DEP_DIR)/*

# Compiling files
%.o: %.cc Makefile
	$(quiet_cc)
	$(Q) $(CXX) $(CXXFLAGS) -c $<

# Build and include header file dependencies
$(DEP_DIR)/%.d: %.cc
	@$(CXX) $(CXXFLAGS) -M $< > $@.$$$$;                   \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;    \
	rm -f $@.$$$$
-include $(OBJS:%.o=$(DEP_DIR)/%.d)

# Define  pretty printing functions
ifeq (${VERBOSE},1)
	quiet_cc =
	quiet_ld =
	Q =
else
	quiet_cc = @echo "  CC " $<
	quiet_ld = @echo "  LD " $@
	Q = @
endif

