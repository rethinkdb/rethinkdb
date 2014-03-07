//
//! Copyright (c) 2011
//! Brandon Kohn
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/operators.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/comparison/less.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/test/minimal.hpp>

//! Generate default traits for the specified source and target.
#define BOOST_NUMERIC_CONVERSION_GENERATE_CAST_TRAITS(r, state)    \
template <>                                                        \
struct numeric_cast_traits<                                        \
    BOOST_PP_SEQ_ELEM( BOOST_PP_TUPLE_ELEM(4,0,state)              \
                     , BOOST_PP_TUPLE_ELEM(4,3,state) )            \
  , BOOST_PP_TUPLE_ELEM(4,2,state)>                                \
{                                                                  \
    typedef def_overflow_handler    overflow_policy;               \
    typedef UseInternalRangeChecker range_checking_policy;         \
    typedef Trunc<BOOST_PP_TUPLE_ELEM(4,2,state)> rounding_policy; \
};                                                                 \
/***/
    
#define BOOST_NUMERIC_CONVERSION_TUPLE_SENTINAL(r, state) \
   BOOST_PP_LESS                                          \
   (                                                      \
      BOOST_PP_TUPLE_ELEM(4,0,state)                      \
    , BOOST_PP_TUPLE_ELEM(4,1,state)                      \
   )                                                      \
/***/

#define BOOST_NUMERIC_CONVERSION_INC_OP(r, state)    \
     (                                               \
        BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(4,0,state)) \
      , BOOST_PP_TUPLE_ELEM(4,1,state)               \
      , BOOST_PP_TUPLE_ELEM(4,2,state)               \
      , BOOST_PP_TUPLE_ELEM(4,3,state)               \
     )                                               \
/***/

#define BOOST_NUMERIC_CONVERSION_GENERATE_CAST_TARGET_STEP(r, state)                         \
    BOOST_PP_FOR                                                                             \
    (                                                                                        \
        (                                                                                    \
            0                                                                                \
          , BOOST_PP_TUPLE_ELEM(4,1,state)                                                   \
          , BOOST_PP_SEQ_ELEM(BOOST_PP_TUPLE_ELEM(4,0,state),BOOST_PP_TUPLE_ELEM(4,2,state)) \
          , BOOST_PP_TUPLE_ELEM(4,2,state)                                                   \
        )                                                                                    \
      , BOOST_NUMERIC_CONVERSION_TUPLE_SENTINAL                                              \
      , BOOST_NUMERIC_CONVERSION_INC_OP                                                      \
      , BOOST_NUMERIC_CONVERSION_GENERATE_CAST_TRAITS                                        \
    )                                                                                        \
/***/

#define BOOST_NUMERIC_CONVERSION_GENERATE_BUILTIN_CAST_TRAITS(types) \
    BOOST_PP_FOR                                                     \
    (                                                                \
        (0,BOOST_PP_SEQ_SIZE(types),types,_)                         \
      , BOOST_NUMERIC_CONVERSION_TUPLE_SENTINAL                      \
      , BOOST_NUMERIC_CONVERSION_INC_OP                              \
      , BOOST_NUMERIC_CONVERSION_GENERATE_CAST_TARGET_STEP           \
    )                                                                \
/***/

namespace boost { namespace numeric {
#if !defined( BOOST_NO_INT64_T )
    //! Generate the specializations for the built-in types.
    BOOST_NUMERIC_CONVERSION_GENERATE_BUILTIN_CAST_TRAITS
    (
        (char)
        (boost::int8_t)
        (boost::uint8_t)
        (boost::int16_t)
        (boost::uint16_t)
        (boost::int32_t)
        (boost::uint32_t)
        (boost::int64_t)
        (boost::uint64_t)
        (float)
        (double)
        (long double)
    )
#else
    BOOST_NUMERIC_CONVERSION_GENERATE_BUILTIN_CAST_TRAITS
    (
        (char)
        (boost::int8_t)
        (boost::uint8_t)
        (boost::int16_t)
        (boost::uint16_t)
        (boost::int32_t)
        (boost::uint32_t)
        (float)
        (double)
        (long double)
    )
#endif
}}//namespace boost::numeric;

int test_main( int argc, char * argv[] )
{
    //! This test should not compile.
    return 1;
}

#undef BOOST_NUMERIC_CONVERSION_GENERATE_BUILTIN_CAST_TRAITS
#undef BOOST_NUMERIC_CONVERSION_GENERATE_CAST_TARGET_STEP
#undef BOOST_NUMERIC_CONVERSION_INC_OP
#undef BOOST_NUMERIC_CONVERSION_TUPLE_SENTINAL
#undef BOOST_NUMERIC_CONVERSION_GENERATE_CAST_TRAITS
