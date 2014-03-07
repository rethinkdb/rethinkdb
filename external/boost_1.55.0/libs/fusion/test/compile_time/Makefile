#=============================================================================
#    Copyright (c) 2008 Dan Marsden
#  
#    Use modification and distribution are subject to the Boost Software 
#    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
#    http://www.boost.org/LICENSE_1_0.txt).
#==============================================================================

CXX=g++
CXXFLAGS=-I$(BOOST_ROOT)

TEST_SRCS=\
	vector_construction.cpp\
	vector_iteration.cpp\
	vector_intrinsic.cpp\
	fold.cpp\
	transform.cpp

TEST_OBJS=$(TEST_SRCS:.cpp=.o)

TEST_TARGETS=$(TEST_SRCS:.cpp=.test)

all: $(TEST_TARGETS)

%.test : %.cpp
	time $(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TEST_TARGETS) $(TEST_OBJS)
