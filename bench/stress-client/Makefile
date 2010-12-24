
LIBS = -lpthread -ldl
SRC = main.cc utils.cc random.cc sqlite3.c

MYSQL ?= 1
LIBMEMCACHED ?= 1

ifeq ($(MYSQL),1)
    INCLUDE += -I /usr/local/mysql/include
    LIBS += -lmysqlclient -L /usr/local/mysql/lib/mysql 
    DEFINES += -DUSE_MYSQL
endif

ifeq ($(LIBMEMCACHED),1)
    INCLUDE += -I../../../lib/libmemcached
    LIBS += -lmemcached
    DEFINES += -DUSE_LIBMEMCACHED
endif

EXEC_NAME = stress
CXX = g++
CXXFLAGS = -g $(INCLUDE) -O3 $(DEFINES)

OBJ = $(addsuffix .o, $(basename $(SRC)))

build: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(EXEC_NAME) $(OBJ) -lm -lrt $(TLIB) $(LIBS)

clean:
	rm -f *~
	rm -f *.o
	rm -f $(EXEC_NAME)
