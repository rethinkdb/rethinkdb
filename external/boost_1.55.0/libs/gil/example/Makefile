# Copyright 2008 Lubomir Bourdev and Hailin Jin
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

.SUFFIXES: .cpp 
#CXX=/usr/local/gcc-411/bin/g++
CXX=g++
CXX_FLAGS=-Wall -O2 -DNDEBUG -DBOOST_GIL_USE_CONCEPT_CHECK

BOOST_INCLUDE_PATH=-I../../.. -I../../../../boost_libraries
LIBJPEG_INCLUDE_PATH=-I../../../../lib/libjpeg
LIBJPEG_LIB_PATH=-L../../../../lib/libjpeg

all: resize affine convolution mandelbrot x_gradient histogram dynamic_image interleaved_ptr packed_pixel
.cpp.o:
	${CXX} ${CXX_FLAGS} ${BOOST_INCLUDE_PATH} ${LIBJPEG_INCLUDE_PATH} -c $<
clean:
	-rm -f *.o *.exe 
	-rm -f out-affine.jpg out-resize.jpg out-convolution.jpg out-convolution2.jpg out-mandelbrot.jpg
	-rm -f out-interleaved_ptr.jpg out-x_gradient.jpg out-histogram.txt out-packed_pixel_bgr772.jpg out-packed_pixel_gray1.jpg out-dynamic_image.jpg
resize:	resize.o
	${CXX} -o resize ${CXX_FLAGS} resize.o ${LIBJPEG_LIB_PATH} -ljpeg 
affine:	affine.o
	${CXX} -o affine ${CXX_FLAGS} affine.o ${LIBJPEG_LIB_PATH} -ljpeg
convolution:	convolution.o
	${CXX} -o convolution ${CXX_FLAGS} convolution.o ${LIBJPEG_LIB_PATH} -ljpeg 
mandelbrot:	mandelbrot.o
	${CXX} -o mandelbrot ${CXX_FLAGS} mandelbrot.o ${LIBJPEG_LIB_PATH} -ljpeg 
interleaved_ptr:	interleaved_ptr.o
	${CXX} -o interleaved_ptr ${CXX_FLAGS} interleaved_ptr.o ${LIBJPEG_LIB_PATH} -ljpeg 
interleaved_ptr.cpp:	interleaved_ptr.hpp
x_gradient:	x_gradient.o
	${CXX} -o x_gradient ${CXX_FLAGS} x_gradient.o ${LIBJPEG_LIB_PATH} -ljpeg 
histogram:	histogram.o
	${CXX} -o histogram ${CXX_FLAGS} histogram.o ${LIBJPEG_LIB_PATH} -ljpeg 
packed_pixel:	packed_pixel.o
	${CXX} -o packed_pixel ${CXX_FLAGS} packed_pixel.o ${LIBJPEG_LIB_PATH} -ljpeg 
dynamic_image:	dynamic_image.o
	${CXX} -o dynamic_image ${CXX_FLAGS} dynamic_image.o ${LIBJPEG_LIB_PATH} -ljpeg 

