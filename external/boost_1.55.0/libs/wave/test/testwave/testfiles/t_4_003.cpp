/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests simple expression arithmetics

# define CAT(a, b) PRIMITIVE_CAT(a, b)
# define PRIMITIVE_CAT(a, b) a ## b

# define OFFSET(n) OFFSET_ ## n

# define OFFSET_1 OFFSET_2 % 10
# define OFFSET_2 OFFSET_3 % 100
# define OFFSET_3 OFFSET_4 % 1000
# define OFFSET_4

# define FACTOR(n) FACTOR_ ## n

# define FACTOR_1 / 1
# define FACTOR_2 CAT(FACTOR_1, 0)
# define FACTOR_3 CAT(FACTOR_2, 0)
# define FACTOR_4 CAT(FACTOR_3, 0)

# define DIGIT(n) OFFSET(n) FACTOR(n)

# define x 987

# if (x) DIGIT(3) == 0
#    define D3 0
# elif (x) DIGIT(3) == 1
#    define D3 1
# elif (x) DIGIT(3) == 2
#    define D3 2
# elif (x) DIGIT(3) == 3
#    define D3 3
# elif (x) DIGIT(3) == 4
#    define D3 4
# elif (x) DIGIT(3) == 5
#    define D3 5
# elif (x) DIGIT(3) == 6
#    define D3 6
# elif (x) DIGIT(3) == 7
#    define D3 7
# elif (x) DIGIT(3) == 8
#    define D3 8
# elif (x) DIGIT(3) == 9
#    define D3 9
# endif

# if (x) DIGIT(2) == 0
#    define D2 0
# elif (x) DIGIT(2) == 1
#    define D2 1
# elif (x) DIGIT(2) == 2
#    define D2 2
# elif (x) DIGIT(2) == 3
#    define D2 3
# elif (x) DIGIT(2) == 4
#    define D2 4
# elif (x) DIGIT(2) == 5
#    define D2 5
# elif (x) DIGIT(2) == 6
#    define D2 6
# elif (x) DIGIT(2) == 7
#    define D2 7
# elif (x) DIGIT(2) == 8
#    define D2 8
# elif (x) DIGIT(2) == 9
#    define D2 9
# endif

# if (x) DIGIT(1) == 0
#    define D1 0
# elif (x) DIGIT(1) == 1
#    define D1 1
# elif (x) DIGIT(1) == 2
#    define D1 2
# elif (x) DIGIT(1) == 3
#    define D1 3
# elif (x) DIGIT(1) == 4
#    define D1 4
# elif (x) DIGIT(1) == 5
#    define D1 5
# elif (x) DIGIT(1) == 6
#    define D1 6
# elif (x) DIGIT(1) == 7
#    define D1 7
# elif (x) DIGIT(1) == 8
#    define D1 8
# elif (x) DIGIT(1) == 9
#    define D1 9
# endif

//R #line 100 "t_4_003.cpp"
D3 D2 D1      //R 9 8 7 
