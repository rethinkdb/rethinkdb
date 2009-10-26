CC=g++
CXX=g++
CXXFLAGS=-g -O2
LDFLAGS=-lrt -lm -laio

main: main.o utils.o worker_pool.o event_queue.o async_io.o alloc_blackhole.o

main.o: utils.hpp worker_pool.hpp async_io.hpp alloc_blackhole.o
worker_pool.o: worker_pool.hpp event_queue.hpp utils.hpp
event_queue.o: utils.hpp event_queue.hpp config.hpp alloc_blackhole.hpp
async_io.o: async_io.hpp event_queue.hpp utils.hpp
utils.o: utils.hpp
alloc_blackhole: alloc_blackhole.hpp utils.hpp

clean:
	rm -f main *~ *.o
