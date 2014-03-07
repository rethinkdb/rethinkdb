/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if defined(_MSC_VER)
# pragma warning(disable: 4345)
#endif

#include "expression_def.hpp"

typedef std::string::const_iterator iterator_type;
template struct client::parser::expression<iterator_type>;
