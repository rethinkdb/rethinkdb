/*=============================================================================
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/fusion/tuple.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/adapted/mpl.hpp>

#define FUSION_SEQUENCE tuple
#define FUSION_AT get
#define FUSION_SIZE tuple_size
#include "misc.hpp"

int
main()
{
    test();
    return boost::report_errors();
}

