/*
 * fusion_runge_kutta.hpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef FUSION_EXPLICIT_RK_HPP_
#define FUSION_EXPLICIT_RK_HPP_

#include <boost/mpl/vector.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/size_t.hpp>

#include <boost/fusion/container.hpp>
#include <boost/fusion/algorithm/iteration.hpp>

#include <boost/array.hpp>

#include "fusion_algebra.hpp"
//#include "fusion_foreach_performance.hpp"

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

using namespace std;

struct intermediate_stage {};
struct last_stage {};



template< class T , class Constant >
struct array_wrapper
{
    typedef const typename boost::array< T , Constant::value > type;
};

template< class T , size_t i , class StageCategory >
struct stage
{
    T c;
    boost::array< T , i > a;
    typedef StageCategory category;
};

template< class T , size_t i>
struct stage< T , i , last_stage >
{
    T c;
    boost::array< T , i > b;
    typedef last_stage category;
};



template< class T , class Constant , class StageCategory >
struct stage_wrapper
{
    typedef stage< T , Constant::value , StageCategory > type;
};


template< class StateType , size_t stage_count >
class explicit_rk
{

public:

    typedef StateType state_type;

    typedef mpl::range_c< size_t , 1 , stage_count > stage_indices;

    typedef typename fusion::result_of::as_vector
    <
        typename mpl::copy
        <
            stage_indices ,
            mpl::inserter
            <
                mpl::vector0< > ,
                mpl::push_back< mpl::_1 , array_wrapper< double , mpl::_2 > >
            >
        >::type
    >::type coef_a_type;

    typedef boost::array< double , stage_count > coef_b_type;
    typedef boost::array< double , stage_count > coef_c_type;

    typedef typename fusion::result_of::as_vector
    <
        typename mpl::push_back
        <
            typename mpl::copy
            <
                stage_indices,
                mpl::inserter
                <
                    mpl::vector0<> ,
                    mpl::push_back< mpl::_1 , stage_wrapper< double , mpl::_2 , intermediate_stage > >
                >
            >::type ,
            stage< double , stage_count , last_stage >
        >::type
    >::type stage_vector_base;


    struct stage_vector : public stage_vector_base
    {
        struct do_insertion
        {
            stage_vector_base &m_base;
            const coef_a_type &m_a;
            const coef_c_type &m_c;

            do_insertion( stage_vector_base &base , const coef_a_type &a , const coef_c_type &c )
            : m_base( base ) , m_a( a ) , m_c( c ) { }

            template< class Index >
            void operator()( Index ) const
            {
                //fusion::at< Index >( m_base ) = stage< double , Index::value+1 , intermediate_stage >( m_c[ Index::value ] , fusion::at< Index >( m_a ) );
                fusion::at< Index >( m_base ).c  = m_c[ Index::value ];
                fusion::at< Index >( m_base ).a = fusion::at< Index >( m_a );
            }
        };

        stage_vector( const coef_a_type &a , const coef_b_type &b , const coef_c_type &c )
        {
            typedef mpl::range_c< size_t , 0 , stage_count - 1 > indices;
            mpl::for_each< indices >( do_insertion( *this , a , c ) );
            //fusion::at_c< 0 >( fusion::at_c< stage_count - 1 >( *this ) ) = stage_count - 1 ;
            fusion::at_c< stage_count - 1 >( *this ).c = c[ stage_count - 1 ];
            fusion::at_c< stage_count - 1 >( *this ).b = b;
        }
    };



    template< class System >
    struct calculate_stage
    {
        System &system;
        state_type &x , &x_tmp;
        state_type *F;
        const double t;
        const double dt;

        calculate_stage( System &_system , state_type &_x , state_type &_x_tmp , state_type *_F ,
                            const double _t , const double _dt )
        : system( _system ) , x( _x ) , x_tmp( _x_tmp ) , F( _F ) , t( _t ) , dt( _dt )
        {}


        template< typename T , size_t stage_number >
        void inline operator()( stage< T , stage_number , intermediate_stage > const &stage ) const
        //typename stage_fusion_wrapper< T , mpl::size_t< stage_number > , intermediate_stage >::type const &stage ) const
        {
            if( stage_number == 1 )
                system( x , F[stage_number-1] , t + stage.c * dt );
            else
                system( x_tmp , F[stage_number-1] , t + stage.c * dt );

            fusion_algebra<stage_number>::foreach( x_tmp , x , stage.a , F , dt);
        }


        template< typename T , size_t stage_number >
        void inline operator()( stage< T , stage_number , last_stage > const &stage ) const
        //void operator()( typename stage_fusion_wrapper< T , mpl::size_t< stage_number > , last_stage >::type const &stage ) const
        {
            if( stage_number == 1 )
                system( x , F[stage_number-1] , t + stage.c * dt );
            else
                system( x_tmp , F[stage_number-1] , t + stage.c * dt );

            fusion_algebra<stage_number>::foreach( x , x , stage.b , F , dt);
        }


    };

public:

    explicit_rk( const coef_a_type &a ,
                  const coef_b_type &b ,
                  const coef_c_type &c )
    : m_stages( a , b , c )

    { }


    template< class System >
    void inline do_step( System system , state_type &x , const double t , const double dt )
    {
        fusion::for_each( m_stages , calculate_stage< System >( system , x , m_x_tmp , m_F , t , dt ) );
    }

private:

    stage_vector m_stages;
    state_type m_x_tmp;

protected:
    state_type m_F[stage_count];

};

#endif /* FUSION_EXPLICIT_RK_HPP_ */
