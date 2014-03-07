# Copyright 2009-2012 Karsten Ahnert
# Copyright 2009-2012 Mario Mulansky
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or
# copy at http://www.boost.org/LICENSE_1_0.txt)



# CUDA_ROOT = /home/karsten/boost/cuda4.1/cuda/
CUDA_ROOT = /usr/local/cuda
ARCH = sm_13

CC = gcc-4.4
CXX = gcc-4.4
NVCC = $(CUDA_ROOT)/bin/nvcc
# NVCC = g++

INCLUDES += -I$(BOOST_ROOT) -I$(THRUST_ROOT) -I$(CUDA_ROOT)/include -I../../../../..

NVCCFLAGS = -O3 $(INCLUDES) -arch $(ARCH) --compiler-bindir=/usr/bin/g++-4.4  -Xcompiler -fopenmp -DTHRUST_DEVICE_BACKEND=THRUST_DEVICE_BACKEND_OMP
# NVCCFLAGS = -O3 $(INCLUDES) -arch $(ARCH) --compiler-bindir=/usr/bin/g++-4.3 -Xcompiler -fopenmp -DTHRUST_DEVICE_BACKEND=THRUST_DEVICE_BACKEND_OMP 

# NVCCFLAGS = -O3 $(INCLUDES) -arch $(ARCH) --compiler-bindir=/usr/bin/g++-4.3

#--compiler-bindir=/usr/bin/g++-4.4 
#-Xcompiler -fopenmp -DTHRUST_DEVICE_BACKEND=THRUST_DEVICE_BACKEND_OMP 
# NVCCFLAGS = -O3 $(INCLUDES) -arch $(ARCH) --compiler-bindir=/usr/bin/g++-4.3 
#-Xcompiler -fopenmp -DTHRUST_DEVICE_BACKEND=THRUST_DEVICE_BACKEND_OMP 


LDLIBS = -lcudart -lgomp
LDFLAGS = -L$(CUDA_ROOT)/lib64

%.co : %.cu
	$(NVCC) $(NVCCFLAGS) -c $< -o $@


all : phase_oscillator_chain phase_oscillator_ensemble lorenz_parameters relaxation


phase_oscillator_chain.co : phase_oscillator_chain.cu
phase_oscillator_chain : phase_oscillator_chain.co
	$(CC) -o phase_oscillator_chain $(LDFLAGS) $(LDLIBS) phase_oscillator_chain.co



phase_oscillator_ensemble.co : phase_oscillator_ensemble.cu
phase_oscillator_ensemble : phase_oscillator_ensemble.co
	$(CC) -o phase_oscillator_ensemble $(LDFLAGS) $(LDLIBS) phase_oscillator_ensemble.co



lorenz_parameters : lorenz_parameters.co
	$(CC) -o lorenz_parameters $(LDFLAGS) $(LDLIBS) lorenz_parameters.co
lorenz_parameters.co : lorenz_parameters.cu


relaxation : relaxation.co
	$(CC) -o relaxation $(LDFLAGS) $(LDLIBS) relaxation.co
relaxation.co : relaxation.cu



clean :
	-rm *~ *.o *.co phase_oscillator_chain phase_oscillator_ensemble lorenz_parameters relaxation 
