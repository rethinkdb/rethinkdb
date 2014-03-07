
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#if !defined(BOOST_FT_PREPROCESSING_MODE)

#   ifndef __WAVE__
#     error "Boost.Wave preprocessor required"
#   endif

#   include <boost/preprocessor/seq/cat.hpp>
#   include <boost/preprocessor/stringize.hpp>

#   if BOOST_PP_NIL // enable dependency scanning for dynamically included files
#     include <boost/function_types/detail/encoding/def.hpp>
#     include <boost/function_types/detail/components_impl/master.hpp>
#     include <boost/function_types/detail/synthesize_impl/master.hpp>
#     include <boost/function_types/detail/classifier_impl/master.hpp>
#   endif

#   pragma wave option(line: 0, preserve: 2)
timestamp file
#   pragma wave option(output: null)

#   define BOOST_FT_PREPROCESSING_MODE

#   define BOOST_FT_HEADER \
        BOOST_PP_SEQ_CAT((arity)(BOOST_FT_MAX_ARITY)(_)(BOOST_FT_mfp)).hpp
    #define BOOST_FT_OUT_FILE \
        BOOST_PP_STRINGIZE(../../../BOOST_FT_al_path/BOOST_FT_HEADER)

#   define BOOST_FT_al_path boost/function_types/detail/components_impl
#   include __FILE__
#   undef  BOOST_FT_al_path

#   define BOOST_FT_al_path boost/function_types/detail/synthesize_impl
#   include __FILE__
#   undef  BOOST_FT_al_path

#   define BOOST_FT_al_path boost/function_types/detail/classifier_impl
#   include __FILE__
#   undef  BOOST_FT_al_path

#elif !defined(BOOST_FT_mfp)

#   define BOOST_FT_mfp 0
#   include __FILE__
#   undef  BOOST_FT_mfp

#   define BOOST_FT_mfp 1
#   include __FILE__
#   undef  BOOST_FT_mfp

#elif !defined(BOOST_FT_MAX_ARITY)

#   define BOOST_FT_FROM_ARITY 0
#   define BOOST_FT_MAX_ARITY 10
#   include __FILE__

#   define BOOST_FT_FROM_ARITY 10
#   define BOOST_FT_MAX_ARITY 20
#   include __FILE__

#   define BOOST_FT_FROM_ARITY 20
#   define BOOST_FT_MAX_ARITY 30
#   include __FILE__

#   define BOOST_FT_FROM_ARITY 30
#   define BOOST_FT_MAX_ARITY 40
#   include __FILE__

#   define BOOST_FT_FROM_ARITY 40
#   define BOOST_FT_MAX_ARITY 50
#   include __FILE__

#else

#   pragma message(generating BOOST_FT_OUT_FILE)
#   pragma wave option(preserve: 2, output: BOOST_FT_OUT_FILE)
#   include <boost/function_types/detail/pp_arity_loop.hpp>
#   undef  BOOST_FT_MAX_ARITY

#endif

