
INCLUDE = -I../../../lib/libmemcached -I /usr/local/mysql/include
LIBS = -L. -lmemcached -lmysqlclient -L /usr/local/mysql/lib/mysql
SRC = main.cc utils.cc

EXEC_NAME = stress
CXX = g++
CXXFLAGS = -g $(INCLUDE) -O3

OBJ = $(addsuffix .o, $(basename $(SRC)))

build: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(EXEC_NAME) $(OBJ) -lm -lrt $(TLIB) $(LIBS)

clean:
	rm -f *~
	rm -f *.o
	rm -f $(EXEC_NAME)
