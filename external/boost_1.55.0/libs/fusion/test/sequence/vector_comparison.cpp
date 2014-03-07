/*=============================================================================
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/fusion/container/vector/vector.hpp>

#define FUSION_SEQUENCE vector
#include "comparison.hpp"

int
main()
{
    equality_test();
    ordering_test();
    return boost::report_errors();
}
