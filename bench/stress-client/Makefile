# Copyright 2010-2012 RethinkDB, all rights reserved.
.PHONY: build
UNAME := $(shell uname)
LIBS = -lpthread -ldl
# Do not include main.cc or python_interface.cc in SRC
SRC = utils.cc random.cc protocol.cc protocols/sqlite3.c
HEADERS = $(wildcard *.hpp) $(wildcard */*.hpp) $(wildcard *.h) $(wildcard */*.h)

MYSQL ?= 0
LIBMEMCACHED ?= 0
LIBGSL ?= 0
TAGS=.tags

ifeq ($(MYSQL),1)
INCLUDE += -I /usr/local/mysql/include
LIBS += -lmysqlclient -L /usr/local/mysql/lib/mysql 
DEFINES += -DUSE_MYSQL
endif

ifeq ($(LIBMEMCACHED),1)
INCLUDE += -I../../lib/libmemcached
LIBS += -lmemcached -L /usr/local/lib
DEFINES += -DUSE_LIBMEMCACHED
endif

ifeq ($(LIBGSL),1)
LIBS += -lgsl -lgslcblas
DEFINES += -DUSE_LIBGSL
endif

ifneq ($(UNAME),Darwin)
LIBS += -lrt
endif

EXEC_NAME = stress
SO_NAME = libstress.so
CXX = g++
CC = gcc
CFLAGS = -fPIC
CXXFLAGS = -Wall -Wextra -Werror $(INCLUDE) -I . $(DEFINES) -fPIC -g # -O3

OBJ = $(addsuffix .o, $(basename $(SRC)))

build: $(EXEC_NAME) $(SO_NAME)

%.o: %.cc $(HEADERS) Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(EXEC_NAME): $(OBJ) main.o $(HEADERS) Makefile
	$(CXX) $(CXXFLAGS) -o $(EXEC_NAME) $(OBJ) main.o -lm $(TLIB) $(LIBS)

$(SO_NAME): $(OBJ) python_interface.o $(HEADERS) python_interface.h Makefile
	$(CXX) $(CXXFLAGS) -shared -o $(SO_NAME) $(OBJ) python_interface.o -lm $(TLIB) $(LIBS)

tags: Makefile
	ctags -R -f $(TAGS) --langmap="c++:.cc.tcc.hpp"

clean:
	rm -f *~
	rm -f *.o
	rm -f */*.o
	rm -f $(EXEC_NAME)
	rm -f $(SO_NAME)
