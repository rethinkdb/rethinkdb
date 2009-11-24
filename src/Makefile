CC=g++
CXX=g++
CXXFLAGS=-g -O2
LDFLAGS=-lrt -lm -laio

all: main pool_test
main: main.o utils.o worker_pool.o event_queue.o async_io.o blackhole_alloc.o
pool_test: pool_test.o utils.o

main.o: utils.hpp worker_pool.hpp async_io.hpp blackhole_alloc.o event_queue.hpp
worker_pool.o: worker_pool.hpp event_queue.hpp utils.hpp
event_queue.o: utils.hpp event_queue.hpp config.hpp blackhole_alloc.hpp
async_io.o: async_io.hpp event_queue.hpp utils.hpp blackhole_alloc.hpp
utils.o: utils.hpp
blackhole_alloc: blackhole_alloc.hpp utils.hpp

clean:
	rm -f main pool_test *~ *.o
