
INCLUDE = -I../../../lib/libmemcached -I /usr/local/mysql/include
LIBS = -L. -lmemcached -lmysqlclient -L /usr/local/mysql/lib/mysql -lpthread -ldl
SRC = main.cc utils.cc random.cc sqlite3.c

STATIC ?= 0

EXEC_NAME = stress
CXX = g++
CXXFLAGS = -g $(INCLUDE) -O3

ifeq ($(STATIC),1)
    CXXFLAGS += -static-libgcc
endif

OBJ = $(addsuffix .o, $(basename $(SRC)))

build: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(EXEC_NAME) $(OBJ) -lm -lrt $(TLIB) $(LIBS)

clean:
	rm -f *~
	rm -f *.o
	rm -f $(EXEC_NAME)
