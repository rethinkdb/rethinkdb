/*
 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


/* strongly nonlinear hamiltonian lattice in 2d */

#ifndef LATTICE2D_HPP
#define LATTICE2D_HPP

#include <vector>

#include <boost/math/special_functions/pow.hpp>

using boost::math::pow;

template< int Kappa , int Lambda >
struct lattice2d {

    const double m_beta;
    std::vector< std::vector< double > > m_omega;

    lattice2d( const double beta )
        : m_beta( beta )
    { }

    template< class StateIn , class StateOut >
    void operator()( const StateIn &q , StateOut &dpdt )
    {
        // q and dpdt are 2d
        const int N = q.size();

        int i;
        for( i = 0 ; i < N ; ++i )
        {
            const int i_l = (i-1+N) % N;
            const int i_r = (i+1) % N;
            for( int j = 0 ; j < N ; ++j )
            {
            const int j_l = (j-1+N) % N;
            const int j_r = (j+1) % N;
            dpdt[i][j] = - m_omega[i][j] * pow<Kappa-1>( q[i][j] )
                - m_beta * pow<Lambda-1>( q[i][j] - q[i][j_l] )
                - m_beta * pow<Lambda-1>( q[i][j] - q[i][j_r] )
                - m_beta * pow<Lambda-1>( q[i][j] - q[i_l][j] )
                - m_beta * pow<Lambda-1>( q[i][j] - q[i_r][j] );
            }
        }
    }

    template< class StateIn >
    double energy( const StateIn &q , const StateIn &p )
    {
        // q and dpdt are 2d
        const int N = q.size();
        double energy = 0.0;
        int i;
        for( i = 0 ; i < N ; ++i )
        {
            const int i_l = (i-1+N) % N;
            const int i_r = (i+1) % N;
            for( int j = 0 ; j < N ; ++j )
            {
            const int j_l = (j-1+N) % N;
            const int j_r = (j+1) % N;
            energy += p[i][j]*p[i][j] / 2.0
                        + m_omega[i][j] * pow<Kappa>( q[i][j] ) / Kappa
                + m_beta * pow<Lambda>( q[i][j] - q[i][j_l] ) / Lambda / 2
                + m_beta * pow<Lambda>( q[i][j] - q[i][j_r] ) / Lambda / 2
                + m_beta * pow<Lambda>( q[i][j] - q[i_l][j] ) / Lambda / 2
                + m_beta * pow<Lambda>( q[i][j] - q[i_r][j] ) / Lambda / 2;
            }
        }
        return energy;
    }


    template< class StateIn , class StateOut >
    double local_energy( const StateIn &q , const StateIn &p , StateOut &energy )
    {
        // q and dpdt are 2d
        const int N = q.size();
        double e = 0.0;
        int i;
        for( i = 0 ; i < N ; ++i )
        {
            const int i_l = (i-1+N) % N;
            const int i_r = (i+1) % N;
            for( int j = 0 ; j < N ; ++j )
            {
                const int j_l = (j-1+N) % N;
                const int j_r = (j+1) % N;
                energy[i][j] = p[i][j]*p[i][j] / 2.0
                    + m_omega[i][j] * pow<Kappa>( q[i][j] ) / Kappa
                    + m_beta * pow<Lambda>( q[i][j] - q[i][j_l] ) / Lambda / 2
                    + m_beta * pow<Lambda>( q[i][j] - q[i][j_r] ) / Lambda / 2
                    + m_beta * pow<Lambda>( q[i][j] - q[i_l][j] ) / Lambda / 2
                    + m_beta * pow<Lambda>( q[i][j] - q[i_r][j] ) / Lambda / 2;
                e += energy[i][j];
            }
        }
        //rescale
        e = 1.0/e;
        for( i = 0 ; i < N ; ++i )
            for( int j = 0 ; j < N ; ++j )
                energy[i][j] *= e;
        return 1.0/e;
    }

    void load_pot( const char* filename , const double W , const double gap , 
                   const size_t dim )
    {
        std::ifstream in( filename , std::ios::in | std::ios::binary );
        if( !in.is_open() ) {
            std::cerr << "pot file not found: " << filename << std::endl;
            exit(0);
        } else {
            std::cout << "using pot file: " << filename << std::endl;
        }

        m_omega.resize( dim );
        for( int i = 0 ; i < dim ; ++i )
        {
            m_omega[i].resize( dim );
            for( size_t j = 0 ; j < dim ; ++j )
            {
                if( !in.good() )
                {
                    std::cerr << "I/O Error: " << filename << std::endl;
                    exit(0);
                }
                double d;
                in.read( (char*) &d , sizeof(d) );
                if( (d < 0) || (d > 1.0) )
                {
                    std::cerr << "ERROR: " << d << std::endl;
                    exit(0);
                }
                m_omega[i][j] = W*d + gap;
            }
        }

    }

    void generate_pot( const double W , const double gap , const size_t dim )
    {
        m_omega.resize( dim );
        for( size_t i = 0 ; i < dim ; ++i )
        {
            m_omega[i].resize( dim );
            for( size_t j = 0 ; j < dim ; ++j )
            {
                m_omega[i][j] = W*static_cast<double>(rand())/RAND_MAX + gap;
            }
        }
    }

};

#endif
