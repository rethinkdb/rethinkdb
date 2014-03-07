/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_EXAMPLE_STRUCT_TYPE)
#define BOOST_FUSION_EXAMPLE_STRUCT_TYPE

#include <string>

namespace example
{
    struct example_struct
    {
        std::string name;
        int age;
        example_struct(
            const std::string& n,
            int a)
            : name(n), age(a)
        {}
    };
}

#endif
