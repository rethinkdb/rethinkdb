/*
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <vector>

using namespace std;

struct rt_algebra
{
    template< typename T , size_t dim >
    inline static void foreach( boost::array< T , dim > & x_tmp , 
                                const boost::array< T , dim > &x ,
                                //const vector< double > &a ,
                                const double* a ,
                                const boost::array< T , dim > *k_vector , 
                                const double dt , const size_t s )
    {
        for( size_t i=0 ; i<dim ; ++i )
        {
            x_tmp[i] = x[i];
            for( size_t j = 0 ; j<s ; ++j )
                x_tmp[i] += a[j]*dt*k_vector[j][i];
        }
    }
};
