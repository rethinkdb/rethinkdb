
LIBS = -lpthread -ldl 
# Do not include main.cc or python_interface.cc in SRC
SRC = utils.cc random.cc protocol.cc sqlite3.c
HEADERS = $(wildcard *.hpp)

MYSQL ?= 1
LIBMEMCACHED ?= 1
LIBGSL ?= 1

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

ifeq ($(LIBGSL),1)
    LIBS += -lgsl -lgslcblas
    DEFINES += -DUSE_LIBGSL
endif

EXEC_NAME = stress
SO_NAME = libstress.so
CXX = g++
CFLAGS = -fPIC
CXXFLAGS = -g $(INCLUDE) $(DEFINES) -fPIC # -O3

OBJ = $(addsuffix .o, $(basename $(SRC)))

%.o: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXEC_NAME): $(OBJ) main.o $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(EXEC_NAME) $(OBJ) main.o -lm -lrt $(TLIB) $(LIBS)

$(SO_NAME): $(OBJ) python_interface.o $(HEADERS) python_interface.h
	$(CXX) $(CXXFLAGS) -shared -o $(SO_NAME) $(OBJ) python_interface.o -lm -lrt $(TLIB) $(LIBS)

clean:
	rm -f *~
	rm -f *.o
	rm -f $(EXEC_NAME)
