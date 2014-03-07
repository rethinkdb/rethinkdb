/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/config.hpp>

// The std_tuple_iterator adaptor only supports implementations
// using variadic templates
#if !defined(BOOST_NO_VARIADIC_TEMPLATES)

#include <boost/fusion/adapted/std_tuple.hpp>

#define FUSION_SEQUENCE std::tuple
#define FUSION_TRAVERSAL_TAG random_access_traversal_tag
#include "./iterator.hpp"

int
main()
{
    test();
    return boost::report_errors();
}

#else

int
main()
{
    return 0;
}

#endif

