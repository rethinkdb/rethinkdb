/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_TAG_OF_20060222_2052)
#define BOOST_FUSION_TAG_OF_20060222_2052

#include <boost/fusion/support/tag_of_fwd.hpp>
#include "./example_struct_type.hpp"

namespace example
{
    struct example_sequence_tag;
}

namespace boost { namespace fusion { 

namespace traits {
        
    template<>
    struct tag_of<example::example_struct>
    {
        typedef example::example_sequence_tag type;
    };
}}}

#endif
