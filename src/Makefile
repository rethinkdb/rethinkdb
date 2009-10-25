CC=g++
CXX=g++
CXXFLAGS=-g -O2
LDFLAGS=-lrt -lm -laio

main: main.o utils.o worker_pool.o

main.o: utils.hpp worker_pool.hpp
worker_pool.o: worker_pool.hpp utils.hpp
utils.o: utils.hpp

clean:
	rm -f rethinkdb *~ *.o
