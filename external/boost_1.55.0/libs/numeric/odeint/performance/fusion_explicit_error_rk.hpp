/*
 * fusion_explicit_error_rk.hpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef FUSION_EXPLICIT_ERROR_RK_HPP_
#define FUSION_EXPLICIT_ERROR_RK_HPP_

#include "fusion_explicit_rk_new.hpp"
#include "fusion_algebra.hpp"

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

using namespace std;

template< class StateType , size_t stage_count >
class explicit_error_rk : public explicit_rk< StateType , stage_count >
{

public:

    typedef explicit_rk< StateType , stage_count > base;

    typedef StateType state_type;

    typedef typename base::stage_indices stage_indices;

    typedef typename base::coef_a_type coef_a_type;

    typedef typename base::coef_b_type coef_b_type;
    typedef typename base::coef_c_type coef_c_type;

 public:

    explicit_error_rk( const coef_a_type &a ,
                          const coef_b_type &b ,
                          const coef_b_type &b2 ,
                          const coef_c_type &c )
    : base( a , b , c ) , m_b2( b2 )
    { }

    template< class System >
    void inline do_step( System system , state_type &x , const double t , const double dt , state_type &x_err )
    {
        base::do_step( system , x , t , dt );
        // compute error estimate
        fusion_algebra< stage_count >::foreach( x_err , m_b2 , base::m_F , dt );
    }

private:

    const coef_b_type m_b2;
};

#endif /* FUSION_EXPLICIT_ERROR_RK_HPP_ */
