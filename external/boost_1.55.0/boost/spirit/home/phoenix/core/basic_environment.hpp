/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_CORE_BASIC_ENVIRONMENT_HPP
#define PHOENIX_CORE_BASIC_ENVIRONMENT_HPP

#include <boost/spirit/home/phoenix/core/limits.hpp>
#include <boost/mpl/remove.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/mpl.hpp>

namespace boost { namespace phoenix
{
    template <
        BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
            PHOENIX_ARG_LIMIT, typename T, fusion::void_)>
    class basic_environment
    {
        typedef
            mpl::BOOST_PP_CAT(vector, PHOENIX_ARG_LIMIT)<
                BOOST_PP_ENUM_PARAMS(PHOENIX_ARG_LIMIT, T)>
        args_with_void_type;

    public:

        // args_type: the list of types in an mpl::sequence
        typedef typename 
            mpl::remove<args_with_void_type, fusion::void_>::type 
        args_type;
        
        // tie_type: a fusion::tuple of references
        typedef typename
            fusion::result_of::as_vector<
                typename mpl::transform<
                    args_type, boost::add_reference<mpl::_1> 
                >::type
            >::type
        tie_type;

        basic_environment()
            : args_() {}

        template <typename U0>
        explicit basic_environment(U0& _0)
            : args_(_0) {}

        template <typename U0, typename U1>
        basic_environment(U0& _0, U1& _1)
            : args_(_0, _1) {}

        //  Bring in the rest of the constructors
        #include <boost/spirit/home/phoenix/core/detail/basic_environment.hpp>

        tie_type const& 
        args() const
        {
            return args_;
        }

        tie_type& 
        args()
        {
            return args_;
        }

    private:

        tie_type args_;
    };
}}

#endif

