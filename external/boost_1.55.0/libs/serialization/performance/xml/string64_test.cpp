/* /libs/serialization/xml_performance/string25_test.cpp ***********************

(C) Copyright 2010 Bryce Lelbach

Use, modification and distribution is subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
http://www.boost.org/LICENSE_1_0.txt)

*******************************************************************************/

#include <string>

typedef std::string string;

#define BSL_TYPE         string
#define BSL_DEPTH        3
#define BSL_ROUNDS       256
#define BSL_NODE_MAX     4
#define BSL_SAVE_TMPFILE 0

#include "harness.hpp"

BSL_MAIN

