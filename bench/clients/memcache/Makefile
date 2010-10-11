
INCLUDE = -I../../../lib/libmemcached
LIBS = -lmemcached
SRC = main.cc utils.cc

EXEC_NAME = memcache
CXX = g++
CXXFLAGS = -g $(INCLUDE)

OBJ = $(addsuffix .o, $(basename $(SRC)))

build: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(EXEC_NAME) $(OBJ) -lm -lrt $(TLIB) $(LIBS)

clean:
	rm -f *~
	rm -f *.o
	rm -f $(EXEC_NAME)
