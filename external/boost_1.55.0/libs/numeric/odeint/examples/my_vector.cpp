/*
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * Example for self defined vector type.
 */

#include <vector>

#include <boost/numeric/odeint.hpp>

//[my_vector
template< int MAX_N >
class my_vector
{
    typedef std::vector< double > vector;

public:
    typedef vector::iterator iterator;
    typedef vector::const_iterator const_iterator;

public:
    my_vector( const size_t N )
        : m_v( N )
    { 
        m_v.reserve( MAX_N );
    }

    my_vector()
        : m_v()
    {
        m_v.reserve( MAX_N );
    }

// ... [ implement container interface ]
//]
    const double & operator[]( const size_t n ) const
    { return m_v[n]; }

    double & operator[]( const size_t n )
    { return m_v[n]; }

    iterator begin()
    { return m_v.begin(); }

    const_iterator begin() const
    { return m_v.begin(); }

    iterator end()
    { return m_v.end(); }
    
    const_iterator end() const
    { return m_v.end(); }

    size_t size() const 
    { return m_v.size(); }

    void resize( const size_t n )
    { m_v.resize( n ); }

private:
    std::vector< double > m_v;

};

//[my_vector_resizeable
// define my_vector as resizeable

namespace boost { namespace numeric { namespace odeint {

template<size_t N>
struct is_resizeable< my_vector<N> >
{
    typedef boost::true_type type;
    static const bool value = type::value;
};

} } }
//]


typedef my_vector<3> state_type;

void lorenz( const state_type &x , state_type &dxdt , const double t )
{
    const double sigma( 10.0 );
    const double R( 28.0 );
    const double b( 8.0 / 3.0 );

    dxdt[0] = sigma * ( x[1] - x[0] );
    dxdt[1] = R * x[0] - x[1] - x[0] * x[2];
    dxdt[2] = -b * x[2] + x[0] * x[1];
}

using namespace boost::numeric::odeint;

int main()
{
    state_type x(3);
    x[0] = 5.0 ; x[1] = 10.0 ; x[2] = 10.0;

    // my_vector works with range_algebra as it implements 
    // the required parts of a container interface
    // no further work is required

    integrate_const( runge_kutta4< state_type >() , lorenz , x , 0.0 , 10.0 , 0.1 );
}
