CXX=g++
LDFLAGS=-lrt -lm -laio
CXXFLAGS=-g -O2 -Iinclude
SERVER_EXEC_NAME=rethink

OBJS =  main.o \
	async_io.o \
	event_queue.o \
	utils.o \
	worker_pool.o

# High level build targets
$(SERVER_EXEC_NAME): $(OBJS)
	$(quiet_ld)
	$(Q) $(CXX) $(LDFLAGS) $(OBJS) -o $(SERVER_EXEC_NAME)

clean:
	rm -f $(SERVER_EXEC_NAME) pool_test
	rm -rf *~ *.o *.d

# Compiling files
%.o: %.cc Makefile
	$(quiet_cc)
	$(Q) $(CXX) $(CXXFLAGS) -c $<

# Build and include header file dependencies
%.d: %.cc
	@$(CXX) $(CXXFLAGS) -M $< > $@.$$$$;                \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
-include $(OBJS:.o=.d)

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

