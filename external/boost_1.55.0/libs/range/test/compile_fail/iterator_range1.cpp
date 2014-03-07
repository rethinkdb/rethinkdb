// Boost.Range library
//
// Copyright Neil Groves 2011. Use, modification and distribution is subject
// to the Boost Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range
//

#include <boost/range/iterator_range_core.hpp>

namespace iterator_range_test_detail
{
    void check_iterator_range_doesnt_convert_pointers()
    {
        double source[] = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0 };        
        boost::iterator_range<float*> rng = boost::make_iterator_range(source);
    }
}

