# /* Copyright (C) 2002
#  * Housemarque Oy
#  * http://www.housemarque.com
#  *
#  * Distributed under the Boost Software License, Version 1.0. (See
#  * accompanying file LICENSE_1_0.txt or copy at
#  * http://www.boost.org/LICENSE_1_0.txt)
#  */
#
# /* Revised by Paul Mensonides (2002) */
#
# /* See http://www.boost.org for most recent version. */
#
# /* This example implements over 2200 functions for 1-dimensional arithmetic
#  * array manipulation in C.  The idea is to use preprocessor data structures,
#  * lists, and tuples for storing metainformation to be used for generating
#  * the actual C code.
#  *
#  * Who needs templates anyway? :)
#  * 
#  * Compile with any C compiler with a standards conforming preprocessor.
#  */
#
# include <boost/preprocessor/comparison/less.hpp>
# include <boost/preprocessor/control/if.hpp>
# include <boost/preprocessor/list/at.hpp>
# include <boost/preprocessor/list/cat.hpp>
# include <boost/preprocessor/list/for_each_product.hpp>
# include <boost/preprocessor/logical/or.hpp>
# include <boost/preprocessor/tuple/to_list.hpp>
# include <boost/preprocessor/tuple/eat.hpp>
#
# /* Information about C operators */
#
# /* Accessors for the operator datatype. */
# define OP_SYMBOL(O)      BOOST_PP_TUPLE_ELEM(5, 0, O)
# define OP_NAME(O)        BOOST_PP_TUPLE_ELEM(5, 1, O)
# define OP_IS_FLOATING(O) BOOST_PP_TUPLE_ELEM(5, 2, O)
# define OP_IS_LOGICAL(O)  BOOST_PP_TUPLE_ELEM(5, 3, O)
# define OP_IS_SHIFT(O)    BOOST_PP_TUPLE_ELEM(5, 4, O)
#
# /* List of applicative unary operators. */
# define APPLICATIVE_UNARY_OPS \
   BOOST_PP_TUPLE_TO_LIST( \
      3, \
      ( \
         ( ! , logical_not, 1, 1, 0), \
         ( ~ , bitwise_not, 0, 0, 0), \
         ( - , neg,         1, 0, 0) \
      ) \
   ) \
   /**/
#
# /* List of applicative binary operators. */
# define APPLICATIVE_BINARY_OPS \
   BOOST_PP_TUPLE_TO_LIST( \
      18, \
      ( \
         ( *  , mul           ,1 ,0 ,0), \
         ( /  , div           ,1 ,0 ,0), \
         ( %  , mod           ,0 ,0 ,0), \
         ( +  , add           ,1 ,0 ,0), \
         ( -  , sub           ,1 ,0 ,0), \
         ( << , shift_left    ,0 ,0 ,1), \
         ( >> , shift_right   ,0 ,0 ,1), \
         ( <  , less          ,1 ,1 ,0), \
         ( <= , less_equal    ,1 ,1 ,0), \
         ( >= , greater_equal ,1 ,1 ,0), \
         ( >  , greater       ,1 ,1 ,0), \
         ( == , equal         ,1 ,1 ,0), \
         ( != , not_equal     ,1 ,1 ,0), \
         ( &  , bitwise_and   ,0 ,0 ,0), \
         ( |  , bitwise_or    ,0 ,0 ,0), \
         ( ^  , bitwise_xor   ,0 ,0 ,0), \
         ( && , logical_and   ,1 ,1 ,0), \
         ( || , logical_or    ,1 ,1 ,0) \
      ) \
   ) \
   /**/
#
# /* Information about C built-in types. */
#
# /* Accessors for the type datatype. */
# define TYPE_NAME(T)         BOOST_PP_TUPLE_ELEM(4, 0, T)
# define TYPE_ABBREVIATION(T) BOOST_PP_TUPLE_ELEM(4, 1, T)
# define TYPE_IS_FLOATING(T)  BOOST_PP_TUPLE_ELEM(4, 2, T)
# define TYPE_RANK(T)         BOOST_PP_TUPLE_ELEM(4, 3, T)
#
# /* List of C built-in types. */
# define BUILTIN_TYPES \
   BOOST_PP_TUPLE_TO_LIST( \
      12, \
      ( \
         ( signed char    ,sc, 0, 1), \
         ( char           ,ch, 0, 1), \
         ( unsigned char  ,uc, 0, 1), \
         ( short          ,ss, 0, 2), \
         ( unsigned short ,us, 0, 2), \
         TYPE_INT, \
         ( unsigned       ,ui, 0, 4), \
         ( long           ,sl, 0, 5), \
         ( unsigned long  ,ul, 0, 6), \
         ( float          ,fl, 1, 7), \
         ( double         ,db, 1, 8), \
         ( long double    ,ld, 1, 9) \
      ) \
   ) \
   /**/
#
# /* Type int is needed in some type computations. */
# define TYPE_INT (int, si, 0, 3)
#
# /* Type computation macros. */
# define TYPE_OF_INTEGER_PROMOTION(T) \
   BOOST_PP_IF( \
      BOOST_PP_LESS(TYPE_RANK(T), TYPE_RANK(TYPE_INT)), \
      TYPE_INT, T \
   ) \
   /**/
# define TYPE_OF_USUAL_ARITHMETIC_CONVERSION(L, R) \
   TYPE_OF_INTEGER_PROMOTION( \
      BOOST_PP_IF( \
         BOOST_PP_LESS(TYPE_RANK(L), TYPE_RANK(R)), \
         R, L \
      ) \
   ) \
   /**/
# define TYPE_OF_UNARY_OP(O, T) \
   BOOST_PP_IF( \
      OP_IS_LOGICAL(O), \
      TYPE_INT, TYPE_OF_INTEGER_PROMOTION(T) \
   ) \
   /**/
# define TYPE_OF_BINARY_OP(O, L, R) \
   BOOST_PP_IF( \
      OP_IS_LOGICAL(O), TYPE_INT, \
      BOOST_PP_IF( \
         OP_IS_SHIFT(O), \
         TYPE_OF_INTEGER_PROMOTION(L), \
         TYPE_OF_USUAL_ARITHMETIC_CONVERSION(L,R) \
      ) \
   ) \
   /**/
# define IS_VALID_UNARY_OP_AND_TYPE_COMBINATION(O, T) \
   BOOST_PP_IF( \
      TYPE_IS_FLOATING(T), \
      OP_IS_FLOATING(O), 1 \
   ) \
   /**/
# define IS_VALID_BINARY_OP_AND_TYPE_COMBINATION(O, L, R) \
   BOOST_PP_IF( \
      BOOST_PP_OR(TYPE_IS_FLOATING(L), TYPE_IS_FLOATING(R)), \
      OP_IS_FLOATING(O), 1 \
   ) \
   /**/
#
# /* Generates code for all unary operators and integral types. */
# define UNARY_ARRAY_OP(_, OT) \
   BOOST_PP_IF( \
      IS_VALID_UNARY_OP_AND_TYPE_COMBINATION OT, \
      UNARY_ARRAY_OP_CODE, BOOST_PP_TUPLE_EAT(2) \
   ) OT \
   /**/
# define UNARY_ARRAY_OP_CODE(O, T) \
   void BOOST_PP_LIST_CAT(BOOST_PP_TUPLE_TO_LIST(4, (array_, OP_NAME(O), _, TYPE_ABBREVIATION(T)))) \
   (const TYPE_NAME(T)* in, TYPE_NAME(TYPE_OF_UNARY_OP(O, T))* out, unsigned n) { \
      do { \
         *out++ = OP_SYMBOL(O) *in++; \
      } while (--n); \
   } \
   /**/

BOOST_PP_LIST_FOR_EACH_PRODUCT(UNARY_ARRAY_OP, 2, (APPLICATIVE_UNARY_OPS, BUILTIN_TYPES))

# /* Generates code for all binary operators and integral type pairs. */
# define BINARY_ARRAY_OP(_, OLR) \
   BOOST_PP_IF( \
      IS_VALID_BINARY_OP_AND_TYPE_COMBINATION OLR, \
      BINARY_ARRAY_OP_CODE, BOOST_PP_TUPLE_EAT(3) \
   ) OLR \
   /**/
# define BINARY_ARRAY_OP_CODE(O, L, R) \
   void BOOST_PP_LIST_CAT( \
      BOOST_PP_TUPLE_TO_LIST( \
         6, (array_, OP_NAME(O), _, TYPE_ABBREVIATION(L), _, TYPE_ABBREVIATION(R)) \
      ) \
   )(const TYPE_NAME(L)* lhs_in, const TYPE_NAME(R)* rhs_in, TYPE_NAME(TYPE_OF_BINARY_OP(O, L, R))* out, unsigned n) { \
      do { \
         *out++ = *lhs_in OP_SYMBOL(O) *rhs_in; \
         ++lhs_in; \
         ++rhs_in; \
      } while (--n); \
   } \
   /**/

BOOST_PP_LIST_FOR_EACH_PRODUCT(BINARY_ARRAY_OP, 3, (APPLICATIVE_BINARY_OPS, BUILTIN_TYPES, BUILTIN_TYPES))
