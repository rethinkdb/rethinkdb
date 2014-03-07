/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests more complex macro expansion. 

// token-pasting macro
#define CAT(a, b) PRIMITIVE_CAT(a, b)
#define PRIMITIVE_CAT(a, b) a ## b

// splits a value that is about to expand into two parameters and returns 
// either the zero-th or one-th element.
#define SPLIT(n, im) PRIMITIVE_CAT(SPLIT_, n)(im)
#define SPLIT_0(a, b) a
#define SPLIT_1(a, b) b

// detects if the parameter is nullary parentheses () or something else.  
// passing non-nullary parenthesis is invalid input.
#define IS_NULLARY(expr) \
    SPLIT( \
        0, \
        CAT(IS_NULLARY_R_, IS_NULLARY_T expr) \
    ) \
    /**/
#define IS_NULLARY_T() 1
#define IS_NULLARY_R_1 1, ?
#define IS_NULLARY_R_IS_NULLARY_T 0, ?

// expands to a macro that eats an n-element parenthesized expression.
#define EAT(n) PRIMITIVE_CAT(EAT_, n)
#define EAT_0()
#define EAT_1(a)
#define EAT_2(a, b)
#define EAT_3(a, b, c)

// expands to a macro that removes the parentheses from an n-element 
// parenthesized expression
#define REM(n) PRIMITIVE_CAT(REM_, n)
#define REM_0()
#define REM_1(a) a
#define REM_2(a, b) a, b
#define REM_3(a, b, c) a, b, c

// expands to nothing
#define NIL

// expands to 1 if x is less than y otherwise, it expands to 0
#define LESS(x, y) \
    IS_NULLARY( \
        PRIMITIVE_CAT(LESS_, y)( \
            EAT(1), PRIMITIVE_CAT(LESS_, x) \
        )() \
    ) \
    /**/

#define LESS_0(a, b) a(EAT(2)) b(REM(1), NIL)
#define LESS_1(a, b) a(LESS_0) b(REM(1), NIL)
#define LESS_2(a, b) a(LESS_1) b(REM(1), NIL)
#define LESS_3(a, b) a(LESS_2) b(REM(1), NIL)
#define LESS_4(a, b) a(LESS_3) b(REM(1), NIL)
#define LESS_5(a, b) a(LESS_4) b(REM(1), NIL)

// expands to the binary one's compliment of a binary input value.  i.e. 0 or 1
#define COMPL(n) PRIMITIVE_CAT(COMPL_, n)
#define COMPL_0 1
#define COMPL_1 0

// these do the obvious...
#define GREATER(x, y) LESS(y, x)
#define LESS_EQUAL(x, y) COMPL(LESS(y, x))
#define GREATER_EQUAL(x, y) COMPL(LESS(x, y))

// causes another rescan...
#define SCAN(x) x

// expands to 1 if x is not equal to y. this one contains a workaround...
#define NOT_EQUAL(x, y) \
    IS_NULLARY( \
        SCAN( \
            PRIMITIVE_CAT(LESS_, x)( \
                EAT(1), \
                PRIMITIVE_CAT(LESS_, y) EAT(2) \
            )((), ...) \
        ) \
    ) \
    /**/
#define EQUAL(x, y) COMPL(NOT_EQUAL(x, y))

//R #line 95 "t_1_024.cpp"
LESS(2, 3)            //R 1 
LESS(3, 2)            //R 0 
LESS(3, 3)            //R 0 
GREATER(2, 3)         //R 0 
GREATER(3, 2)         //R 1 
GREATER(3, 3)         //R 0 
LESS_EQUAL(2, 3)      //R 1 
LESS_EQUAL(3, 2)      //R 0 
LESS_EQUAL(3, 3)      //R 1 
GREATER_EQUAL(2, 3)   //R 0 
GREATER_EQUAL(3, 2)   //R 1 
GREATER_EQUAL(3, 3)   //R 1 
NOT_EQUAL(2, 3)       //R 1 
NOT_EQUAL(3, 2)       //R 1 
NOT_EQUAL(3, 3)       //R 0 
EQUAL(2, 3)           //R 0 
EQUAL(3, 2)           //R 0 
EQUAL(3, 3)           //R 1 
