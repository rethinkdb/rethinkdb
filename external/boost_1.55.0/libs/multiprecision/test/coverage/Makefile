# copyright John Maddock 2012
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.


SOURCES=\
../../../../boost/multiprecision/mpfr.hpp\
../../../../boost/multiprecision/tommath.hpp\
../../../../boost/multiprecision/number.hpp\
../../../../boost/multiprecision/cpp_dec_float.hpp\
../../../../boost/multiprecision/gmp.hpp\
../../../../boost/multiprecision/random.hpp\
../../../../boost/multiprecision/cpp_int.hpp\
../../../../boost/multiprecision/miller_rabin.hpp\
../../../../boost/multiprecision/rational_adaptor.hpp\
../../../../boost/multiprecision/concepts/mp_number_archetypes.hpp\
../../../../boost/multiprecision/detail/big_lanczos.hpp\
../../../../boost/multiprecision/detail/digits.hpp\
../../../../boost/multiprecision/detail/no_et_ops.hpp\
../../../../boost/multiprecision/detail/number_base.hpp\
../../../../boost/multiprecision/detail/generic_interconvert.hpp\
../../../../boost/multiprecision/detail/number_compare.hpp\
../../../../boost/multiprecision/detail/default_ops.hpp\
../../../../boost/multiprecision/detail/integer_ops.hpp\
../../../../boost/multiprecision/detail/functions/constants.hpp\
../../../../boost/multiprecision/detail/functions/pow.hpp\
../../../../boost/multiprecision/detail/functions/trig.hpp\
../../../../boost/multiprecision/traits/explicit_conversion.hpp\
../../../../boost/multiprecision/traits/is_restricted_conversion.hpp

coverage : run
	gcov test_arithmetic_cpp_int_1 test_arithmetic_cpp_int_2 test_arithmetic_cpp_int_3 \
	gcov test_arithmetic_cpp_int_4 test_arithmetic_cpp_int_5 test_arithmetic_cpp_int_6 \
	gcov test_arithmetic_cpp_int_7 test_arithmetic_cpp_int_8 test_arithmetic_cpp_int_9 \
	gcov test_arithmetic_cpp_int_10 test_arithmetic_cpp_int_11 test_arithmetic_cpp_int_12 \
	gcov test_arithmetic_cpp_int_13 test_arithmetic_cpp_int_14 test_arithmetic_cpp_int_15 \
	gcov test_arithmetic_cpp_int_16 test_arithmetic_cpp_int_17 test_arithmetic_cpp_int_18 \
	test_arithmetic_cpp_dec_float_1 test_arithmetic_cpp_dec_float_2 test_arithmetic_cpp_dec_float_3\
   test_arithmetic_concept test_arithmetic_mpz test_arithmetic_mpf \
	test_arithmetic_mpf_50 test_arithmetic_mpq test_arithmetic_mpfr test_arithmetic_mpfr_50 test_cpp_int \
	test_int_io test_float_io test_rational_io test_gmp_conversions test_mpfr_conversions test_cpp_int_conv

test_arithmetic_cpp_int_1.o : ../test_arithmetic_cpp_int_1.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_1 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_1.o -c ../test_arithmetic_cpp_int_1.cpp

test_arithmetic_cpp_int_1 : test_arithmetic_cpp_int_1.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_1 test_arithmetic_cpp_int_1.o

test_arithmetic_cpp_int_2.o : ../test_arithmetic_cpp_int_2.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_2 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_2.o -c ../test_arithmetic_cpp_int_2.cpp

test_arithmetic_cpp_int_2 : test_arithmetic_cpp_int_2.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_2 test_arithmetic_cpp_int_2.o

test_arithmetic_cpp_int_3.o : ../test_arithmetic_cpp_int_3.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_3.o -c ../test_arithmetic_cpp_int_3.cpp

test_arithmetic_cpp_int_3 : test_arithmetic_cpp_int_3.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_3 test_arithmetic_cpp_int_3.o

test_arithmetic_cpp_int_4.o : ../test_arithmetic_cpp_int_4.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_4.o -c ../test_arithmetic_cpp_int_4.cpp

test_arithmetic_cpp_int_4 : test_arithmetic_cpp_int_4.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_4 test_arithmetic_cpp_int_4.o

test_arithmetic_cpp_int_5.o : ../test_arithmetic_cpp_int_5.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_5.o -c ../test_arithmetic_cpp_int_5.cpp

test_arithmetic_cpp_int_5 : test_arithmetic_cpp_int_5.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_5 test_arithmetic_cpp_int_5.o

test_arithmetic_cpp_int_6.o : ../test_arithmetic_cpp_int_6.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_6.o -c ../test_arithmetic_cpp_int_6.cpp

test_arithmetic_cpp_int_6 : test_arithmetic_cpp_int_6.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_6 test_arithmetic_cpp_int_6.o

test_arithmetic_cpp_int_7.o : ../test_arithmetic_cpp_int_7.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_7.o -c ../test_arithmetic_cpp_int_7.cpp

test_arithmetic_cpp_int_7 : test_arithmetic_cpp_int_7.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_7 test_arithmetic_cpp_int_7.o

test_arithmetic_cpp_int_8.o : ../test_arithmetic_cpp_int_8.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_8.o -c ../test_arithmetic_cpp_int_8.cpp

test_arithmetic_cpp_int_8 : test_arithmetic_cpp_int_8.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_8 test_arithmetic_cpp_int_8.o

test_arithmetic_cpp_int_9.o : ../test_arithmetic_cpp_int_9.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_9.o -c ../test_arithmetic_cpp_int_9.cpp

test_arithmetic_cpp_int_9 : test_arithmetic_cpp_int_9.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_9 test_arithmetic_cpp_int_9.o

test_arithmetic_cpp_int_10.o : ../test_arithmetic_cpp_int_10.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_10.o -c ../test_arithmetic_cpp_int_10.cpp

test_arithmetic_cpp_int_10 : test_arithmetic_cpp_int_10.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_10 test_arithmetic_cpp_int_10.o

test_arithmetic_cpp_int_11.o : ../test_arithmetic_cpp_int_11.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_11.o -c ../test_arithmetic_cpp_int_12.cpp

test_arithmetic_cpp_int_11 : test_arithmetic_cpp_int_11.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_11 test_arithmetic_cpp_int_11.o

test_arithmetic_cpp_int_12.o : ../test_arithmetic_cpp_int_12.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_12.o -c ../test_arithmetic_cpp_int_12.cpp

test_arithmetic_cpp_int_12 : test_arithmetic_cpp_int_12.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_12 test_arithmetic_cpp_int_12.o

test_arithmetic_cpp_int_13.o : ../test_arithmetic_cpp_int_13.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_13.o -c ../test_arithmetic_cpp_int_13.cpp

test_arithmetic_cpp_int_13 : test_arithmetic_cpp_int_13.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_13 test_arithmetic_cpp_int_13.o

test_arithmetic_cpp_int_14.o : ../test_arithmetic_cpp_int_14.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_14.o -c ../test_arithmetic_cpp_int_14.cpp

test_arithmetic_cpp_int_14 : test_arithmetic_cpp_int_14.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_14 test_arithmetic_cpp_int_14.o

test_arithmetic_cpp_int_15.o : ../test_arithmetic_cpp_int_15.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_15.o -c ../test_arithmetic_cpp_int_15.cpp

test_arithmetic_cpp_int_15 : test_arithmetic_cpp_int_15.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_15 test_arithmetic_cpp_int_15.o

test_arithmetic_cpp_int_16.o : ../test_arithmetic_cpp_int_16.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_16.o -c ../test_arithmetic_cpp_int_16.cpp

test_arithmetic_cpp_int_16 : test_arithmetic_cpp_int_16.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_16 test_arithmetic_cpp_int_16.o

test_arithmetic_cpp_int_17.o : ../test_arithmetic_cpp_int_17.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_17.o -c ../test_arithmetic_cpp_int_17.cpp

test_arithmetic_cpp_int_17 : test_arithmetic_cpp_int_17.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_17 test_arithmetic_cpp_int_17.o

test_arithmetic_cpp_int_18.o : ../test_arithmetic_cpp_int_18.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_INT_3 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_int_18.o -c ../test_arithmetic_cpp_int_18.cpp

test_arithmetic_cpp_int_18 : test_arithmetic_cpp_int_18.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_int_18 test_arithmetic_cpp_int_18.o

test_arithmetic_cpp_dec_float_1.o : ../test_arithmetic_cpp_dec_float_1.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_DEC_FLOAT --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_dec_float_1.o -c ../test_arithmetic_cpp_dec_float_1.cpp

test_arithmetic_cpp_dec_float_1 : test_arithmetic_cpp_dec_float_1.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_dec_float_1 test_arithmetic_cpp_dec_float_1.o

test_arithmetic_cpp_dec_float_2.o : ../test_arithmetic_cpp_dec_float_2.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_DEC_FLOAT --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_dec_float_2.o -c ../test_arithmetic_cpp_dec_float_2.cpp

test_arithmetic_cpp_dec_float_2 : test_arithmetic_cpp_dec_float_2.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_dec_float_2 test_arithmetic_cpp_dec_float_2.o

test_arithmetic_cpp_dec_float_3.o : ../test_arithmetic_cpp_dec_float_3.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_CPP_DEC_FLOAT --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_cpp_dec_float_3.o -c ../test_arithmetic_cpp_dec_float_3.cpp

test_arithmetic_cpp_dec_float_3 : test_arithmetic_cpp_dec_float_3.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_cpp_dec_float_3 test_arithmetic_cpp_dec_float_3.o

test_arithmetic_concept.o : ../test_arithmetic_backend_concept.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_BACKEND --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_concept.o -c ../test_arithmetic_backend_concept.cpp

test_arithmetic_concept : test_arithmetic_concept.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_concept test_arithmetic_concept.o

test_arithmetic_mpz.o : ../test_arithmetic_mpz.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_MPZ --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_mpz.o -c ../test_arithmetic_mpz.cpp

test_arithmetic_mpz : test_arithmetic_mpz.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_mpz test_arithmetic_mpz.o -lgmp

test_arithmetic_mpf.o : ../test_arithmetic_mpf.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_MPF --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_mpf.o -c ../test_arithmetic_mpf.cpp

test_arithmetic_mpf : test_arithmetic_mpf.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_mpf test_arithmetic_mpf.o -lgmp

test_arithmetic_mpf_50.o : ../test_arithmetic_mpf_50.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_MPF_50 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_mpf_50.o -c ../test_arithmetic_mpf_50.cpp

test_arithmetic_mpf_50 : test_arithmetic_mpf_50.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_mpf_50 test_arithmetic_mpf_50.o -lgmp

test_arithmetic_mpq.o : ../test_arithmetic_mpq.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_MPQ --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_mpq.o -c ../test_arithmetic_mpq.cpp

test_arithmetic_mpq : test_arithmetic_mpq.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_mpq test_arithmetic_mpq.o -lgmp

test_arithmetic_mpfr.o : ../test_arithmetic_mpfr.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_MPFR --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_mpfr.o -c ../test_arithmetic_mpfr.cpp

test_arithmetic_mpfr : test_arithmetic_mpfr.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_mpfr test_arithmetic_mpfr.o -lmpfr -lgmp

test_arithmetic_mpfr_50.o : ../test_arithmetic_mpfr_50.cpp $(SOURCES)
	g++ -std=gnu++0x -DTEST_MPFR_50 --coverage -g -I../../../.. -I../../../../../../trunk -o test_arithmetic_mpfr_50.o -c ../test_arithmetic_mpfr_50.cpp

test_arithmetic_mpfr_50 : test_arithmetic_mpfr_50.o
	g++ -std=gnu++0x --coverage -o test_arithmetic_mpfr_50 test_arithmetic_mpfr_50.o -lmpfr -lgmp

test_cpp_int.o : ../test_cpp_int.cpp $(SOURCES)
	g++ -std=gnu++0x --coverage -g -I../../../.. -I../../../../../../trunk -o test_cpp_int.o -c ../test_cpp_int.cpp

test_cpp_int : test_cpp_int.o
	g++ -std=gnu++0x --coverage -o test_cpp_int test_cpp_int.o -lgmp

test_int_io.o : ../test_int_io.cpp $(SOURCES)
	g++ -std=gnu++0x --coverage -DTEST_MPZ -DTEST_CPP_INT -g -I../../../.. -I../../../../../../trunk -o test_int_io.o -c ../test_int_io.cpp

test_int_io : test_int_io.o
	g++ -std=gnu++0x --coverage -o test_int_io test_int_io.o -lgmp

test_float_io.o : ../test_float_io.cpp $(SOURCES)
	g++ -std=gnu++0x --coverage -g -DTEST_MPF_50 -DTEST_CPP_DEC_FLOAT -DTEST_MPFR_50 -I../../../.. -I../../../../../../trunk -o test_float_io.o -c ../test_float_io.cpp

test_float_io : test_float_io.o
	g++ -std=gnu++0x --coverage -o test_float_io test_float_io.o -lmpfr -lgmp

test_rational_io.o : ../test_rational_io.cpp $(SOURCES)
	g++ -std=gnu++0x --coverage -DTEST_MPQ -DTEST_CPP_INT -g -I../../../.. -I../../../../../../trunk -o test_rational_io.o -c ../test_rational_io.cpp

test_rational_io : test_rational_io.o
	g++ -std=gnu++0x --coverage -o test_rational_io test_rational_io.o -lgmp

test_gmp_conversions.o : ../test_gmp_conversions.cpp $(SOURCES)
	g++ -std=gnu++0x --coverage -g -I../../../.. -I../../../../../../trunk -o test_gmp_conversions.o -c ../test_gmp_conversions.cpp

test_gmp_conversions : test_gmp_conversions.o
	g++ -std=gnu++0x --coverage -o test_gmp_conversions test_gmp_conversions.o -lgmp

test_mpfr_conversions.o : ../test_mpfr_conversions.cpp $(SOURCES)
	g++ -std=gnu++0x --coverage -g -I../../../.. -I../../../../../../trunk -o test_mpfr_conversions.o -c ../test_mpfr_conversions.cpp

test_mpfr_conversions : test_mpfr_conversions.o
	g++ -std=gnu++0x --coverage -o test_mpfr_conversions test_mpfr_conversions.o -lmpfr -lgmp

test_cpp_int_conv.o : ../test_cpp_int_conv.cpp $(SOURCES)
	g++ -std=gnu++0x --coverage -g -I../../../.. -I../../../../../../trunk -o test_cpp_int_conv.o -c ../test_cpp_int_conv.cpp

test_cpp_int_conv : test_cpp_int_conv.o
	g++ -std=gnu++0x --coverage -o test_cpp_int_conv test_cpp_int_conv.o

run : test_arithmetic_cpp_int_1 test_arithmetic_cpp_int_2 test_arithmetic_cpp_int_3 \
      test_arithmetic_cpp_int_4 test_arithmetic_cpp_int_5 test_arithmetic_cpp_int_6 \
      test_arithmetic_cpp_int_7 test_arithmetic_cpp_int_8 test_arithmetic_cpp_int_9 \
      test_arithmetic_cpp_int_10 test_arithmetic_cpp_int_11 test_arithmetic_cpp_int_12 \
      test_arithmetic_cpp_int_13 test_arithmetic_cpp_int_14 test_arithmetic_cpp_int_15 \
      test_arithmetic_cpp_int_16 test_arithmetic_cpp_int_17 test_arithmetic_cpp_int_18 \
      test_arithmetic_cpp_dec_float_1 test_arithmetic_cpp_dec_float_2 test_arithmetic_cpp_dec_float_3 test_arithmetic_concept \
      test_arithmetic_mpz test_arithmetic_mpf test_arithmetic_mpf_50 test_arithmetic_mpq test_arithmetic_mpfr test_arithmetic_mpfr_50 \
      test_cpp_int test_int_io test_float_io test_rational_io test_gmp_conversions test_mpfr_conversions test_cpp_int_conv
	./test_arithmetic_cpp_int_1
	./test_arithmetic_cpp_int_2
	./test_arithmetic_cpp_int_3
	./test_arithmetic_cpp_int_4
	./test_arithmetic_cpp_int_5
	./test_arithmetic_cpp_int_6
	./test_arithmetic_cpp_int_7
	./test_arithmetic_cpp_int_8
	./test_arithmetic_cpp_int_9
	./test_arithmetic_cpp_int_10
	./test_arithmetic_cpp_int_11
	./test_arithmetic_cpp_int_12
	./test_arithmetic_cpp_int_13
	./test_arithmetic_cpp_int_14
	./test_arithmetic_cpp_int_15
	./test_arithmetic_cpp_int_16
	./test_arithmetic_cpp_int_17
	./test_arithmetic_cpp_int_18
	./test_arithmetic_cpp_dec_float_1
	./test_arithmetic_cpp_dec_float_2
	./test_arithmetic_cpp_dec_float_3
	./test_arithmetic_concept
	./test_arithmetic_mpz
	./test_arithmetic_mpf
	./test_arithmetic_mpf_50
	./test_arithmetic_mpq
	./test_arithmetic_mpfr
	./test_arithmetic_mpfr_50
	./test_cpp_int
	./test_int_io
	./test_float_io
	./test_rational_io
	./test_gmp_conversions
	./test_mpfr_conversions
	./test_cpp_int_conv

clean :
	rm -rf *.o *.gc* test_arithmetic_cpp_int_1 test_arithmetic_cpp_int_2 test_arithmetic_cpp_int_3 \
   test_arithmetic_cpp_int_4 test_arithmetic_cpp_int_5 test_arithmetic_cpp_int_6 \
   test_arithmetic_cpp_int_7 test_arithmetic_cpp_int_8 test_arithmetic_cpp_int_9 \
   test_arithmetic_cpp_int_10 test_arithmetic_cpp_int_11 test_arithmetic_cpp_int_12 \
   test_arithmetic_cpp_int_13 test_arithmetic_cpp_int_14 test_arithmetic_cpp_int_15 \
   test_arithmetic_cpp_int_16 test_arithmetic_cpp_int_17 test_arithmetic_cpp_int_18 \
   test_arithmetic_cpp_dec_float test_arithmetic_concept test_arithmetic_mpz \
   test_arithmetic_mpf test_arithmetic_mpf_50 test_arithmetic_mpq test_arithmetic_mpfr test_arithmetic_mpfr_50 test_cpp_int\
   test_int_io test_float_io test_rational_io test_gmp_conversions test_mpfr_conversions test_cpp_int_conv





