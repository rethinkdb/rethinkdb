/*=============================================================================
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/fusion/tuple.hpp>

#define FUSION_SEQUENCE boost::fusion::tuple
#define FUSION_AT get
#define FUSION_MAKE boost::fusion::make_tuple
#include "make.hpp"

int
main()
{
    test();
    return boost::report_errors();
}

