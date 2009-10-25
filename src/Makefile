CC=g++
CXX=g++
CXXFLAGS=-g -O2
LDFLAGS=-lrt -lm -laio

main: main.o utils.o worker_pool.o event_queue.o

main.o: utils.hpp worker_pool.hpp async_io.o
worker_pool.o: worker_pool.hpp event_queue.hpp event_queue_impl.hpp utils.hpp
event_queue.o: utils.hpp event_queue.hpp event_queue_impl.hpp config.hpp
async_io.o: async_io.hpp event_queue.hpp event_queue_impl.hpp utils.hpp
utils.o: utils.hpp

clean:
	rm -f main *~ *.o
