# Copyright 2008 Lubomir Bourdev and Hailin Jin
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

.SUFFIXES: .cpp 
#CXX=/usr/local/gcc4/bin/g++
#CXX=/usr/local/gcc411/bin/g++
CXX=/usr/local/gcc430/bin/g++
#CXX=g++
#CXX='/cygdrive/c/Program Files/Intel/Compiler/C++/10.0.027/IA32/Bin/icl.exe'

CXX_FLAGS=-Wall -DBOOST_GIL_USE_CONCEPT_CHECK #-DNDEBUG 
CXX_CHECKSUM_FLAGS=-Wall -DBOOST_GIL_NO_IO -DBOOST_GIL_USE_CONCEPT_CHECK #-DNDEBUG 

BOOST_INCLUDE_PATH=-I../../.. -I../../../../boost_libraries
LIBJPEG_INCLUDE_PATH=-I../../../../lib/libjpeg
LIBJPEG_LIB_PATH=-L../../../../lib/libjpeg
LIBTIFF_INCLUDE_PATH=-I../../../../lib/libtiff
LIBTIFF_LIB_PATH=-L../../../../lib/libtiff
LIBPNG_INCLUDE_PATH=-I../../../../lib/libpng
LIBPNG_LIB_PATH=-L../../../../lib/libpng
LIBZ_LIB_PATH=-L../../../../lib/zlib

all: performance channel_test pixel_test pixel_iterator_test image_checksum_test image_io_checksum_test
.cpp.o:
#	${CXX} ${CXX_FLAGS} ${BOOST_INCLUDE_PATH} ${LIBJPEG_INCLUDE_PATH} ${LIBTIFF_INCLUDE_PATH} ${LIBPNG_INCLUDE_PATH} -c $<
	${CXX} ${CXX_CHECKSUM_FLAGS} ${BOOST_INCLUDE_PATH} -c $<
clean:
	-rm -f *.o *.exe 
performance: performance.o
	${CXX} -o performance ${CXX_FLAGS} performance.o
channel_test:	channel.o error_if.o
	${CXX} -o channel_test ${CXX_CHECKSUM_FLAGS} channel.o error_if.o
pixel_test:	pixel.o error_if.o
	${CXX} -o pixel_test ${CXX_CHECKSUM_FLAGS} pixel.o error_if.o
pixel_iterator_test:	pixel_iterator.o error_if.o
	${CXX} -o pixel_iterator_test ${CXX_CHECKSUM_FLAGS} pixel_iterator.o error_if.o
image_checksum_test:	image.o error_if.o sample_image.o
	${CXX} -o image_checksum_test ${CXX_CHECKSUM_FLAGS} image.o error_if.o sample_image.o 
image_test:	image.o  error_if.o sample_image.o
	${CXX} -o image_test ${CXX_FLAGS} image.o error_if.o sample_image.o ${LIBJPEG_LIB_PATH} -ljpeg ${LIBTIFF_LIB_PATH} -ltiff ${LIBPNG_LIB_PATH} -lpng ${LIBZ_LIB_PATH} -lz
image_io_checksum_test:	image_io.o error_if.o
	${CXX} -o image_io_checksum_test ${CXX_CHECKSUM_FLAGS} image_io.o error_if.o
image_io_test:	image_io.o error_if.o
	${CXX} -o image_io_test ${CXX_FLAGS} image_io.o error_if.o ${LIBJPEG_LIB_PATH} -ljpeg ${LIBTIFF_LIB_PATH} -ltiff ${LIBPNG_LIB_PATH} -lpng ${LIBZ_LIB_PATH} -lz
run_all: channel_test pixel_test pixel_iterator_test image_checksum_test image_io_checksum_test
	channel_test 
	pixel_test 
	pixel_iterator_test 
	image_checksum_test 
	image_io_checksum_test
