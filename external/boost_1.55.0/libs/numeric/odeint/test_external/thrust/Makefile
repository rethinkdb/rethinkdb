# Copyright 2009-2012 Karsten Ahnert
# Copyright 2009-2012 Mario Mulansky
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or
# copy at http://www.boost.org/LICENSE_1_0.txt)



CUDA_ROOT = /usr/local/cuda

CC = gcc-4.4
CXX = gcc-4.4
NVCC = $(CUDA_ROOT)/bin/nvcc

INCLUDES += -I$(BOOST_ROOT) -I$(THRUST_ROOT) -I$(CUDA_ROOT)/include -I../../../../..

NVCCFLAGS = -O3 $(INCLUDES) --compiler-bindir=/usr/bin/g++-4.4

LDLIBS = -lcudart
LDFLAGS = -L$(CUDA_ROOT)/lib64

%.co : %.cu
	$(NVCC) $(NVCCFLAGS) -o $@ -c $<


all : check_thrust


check_thrust : check_thrust.co
	$(CC) -o check_thrust $(LDFLAGS) $(LDLIBS) check_thrust.co
check_thrust.co : check_thrust.cu

clean :
	-rm *~ *.o *.co check_thrust

