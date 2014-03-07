// Boost seed_rng.hpp header file  ----------------------------------------------//

// Copyright 2007 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  09 Nov 2007 - Initial Revision
//  25 Feb 2008 - moved to namespace boost::uuids::detail
//  28 Nov 2009 - disabled deprecated warnings for MSVC

// seed_rng models a UniformRandomNumberGenerator (see Boost.Random).
// Random number generators are hard to seed well.  This is intended to provide
// good seed values for random number generators.
// It creates random numbers from a sha1 hash of data from a variary of sources,
// all of which are standard function calls.  It produces random numbers slowly.
// Peter Dimov provided the details of sha1_random_digest_().
// see http://archives.free.net.ph/message/20070507.175609.4c4f503a.en.html

#ifndef BOOST_UUID_SEED_RNG_HPP
#define BOOST_UUID_SEED_RNG_HPP

#include <boost/config.hpp>
#include <cstring> // for memcpy
#include <limits>
#include <ctime> // for time_t, time, clock_t, clock
#include <cstdlib> // for rand
#include <cstdio> // for FILE, fopen, fread, fclose
#include <boost/uuid/sha1.hpp>
//#include <boost/nondet_random.hpp> //forward declare boost::random::random_device

// can't use boost::generator_iterator since boost::random number seed(Iter&, Iter)
// functions need a last iterator
//#include <boost/generator_iterator.hpp>
# include <boost/iterator/iterator_facade.hpp>

#if defined(_MSC_VER)
#pragma warning(push) // Save warning settings.
#pragma warning(disable : 4996) // Disable deprecated std::fopen
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
    using ::memcpy;
    using ::time_t;
    using ::time;
    using ::clock_t;
    using ::clock;
    using ::rand;
    using ::FILE;
    using ::fopen;
    using ::fread;
    using ::fclose;
} //namespace std
#endif

// forward declare random number generators
namespace boost { namespace random {
class random_device;
}} //namespace boost::random

namespace boost {
namespace uuids {
namespace detail {

// should this be part of Boost.Random?
class seed_rng
{
public:
    typedef unsigned int result_type;
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    //BOOST_STATIC_CONSTANT(unsigned int, min_value = 0);
    //BOOST_STATIC_CONSTANT(unsigned int, max_value = UINT_MAX);

public:
    // note: rd_ intentionally left uninitialized
    seed_rng()
        : rd_index_(5)
        , random_(std::fopen( "/dev/urandom", "rb" ))
    {}

    ~seed_rng()
    {
        if (random_) {
            std::fclose(random_);
        }
    }

    result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    {
        return (std::numeric_limits<result_type>::min)();
    }
    result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    {
        return (std::numeric_limits<result_type>::max)();
    }

    result_type operator()()
    {
        if (rd_index_ >= 5) {
            //get new digest
            sha1_random_digest_();

            rd_index_ = 0;
        }

        return rd_[rd_index_++];
    }

private:
    inline void ignore_size(size_t) {}

    static unsigned int * sha1_random_digest_state_()
    {
        static unsigned int state[ 5 ];
        return state;
    }

    void sha1_random_digest_()
    {
        boost::uuids::detail::sha1 sha;

        unsigned int * ps = sha1_random_digest_state_();

        unsigned int state[ 5 ];
        std::memcpy( state, ps, sizeof( state ) ); // harmless data race

        sha.process_bytes( (unsigned char const*)state, sizeof( state ) );
        sha.process_bytes( (unsigned char const*)&ps, sizeof( ps ) );

        {
            std::time_t tm = std::time( 0 );
            sha.process_bytes( (unsigned char const*)&tm, sizeof( tm ) );
        }

        {
            std::clock_t ck = std::clock();
            sha.process_bytes( (unsigned char const*)&ck, sizeof( ck ) );
        }

        {
            unsigned int rn[] =
                { static_cast<unsigned int>(std::rand())
                , static_cast<unsigned int>(std::rand())
                , static_cast<unsigned int>(std::rand())
                };
            sha.process_bytes( (unsigned char const*)rn, sizeof( rn ) );
        }

        {
            // intentionally left uninitialized
            unsigned char buffer[ 20 ];

            if(random_)
            {
                ignore_size(std::fread( buffer, 1, 20, random_ ));
            }

            // using an uninitialized buffer[] if fopen fails
            // intentional, we rely on its contents being random
            sha.process_bytes( buffer, sizeof( buffer ) );
        }

        {
            // *p is intentionally left uninitialized
            unsigned int * p = new unsigned int;

            sha.process_bytes( (unsigned char const*)p, sizeof( *p ) );
            sha.process_bytes( (unsigned char const*)&p, sizeof( p ) );

            delete p;
        }

        sha.process_bytes( (unsigned char const*)rd_, sizeof( rd_ ) );

        unsigned int digest[ 5 ];
        sha.get_digest( digest );

        for( int i = 0; i < 5; ++i )
        {
            // harmless data race
            ps[ i ] ^= digest[ i ];
            rd_[ i ] ^= digest[ i ];
        }
    }

private:
    unsigned int rd_[5];
    int rd_index_;
    std::FILE * random_;

private: // make seed_rng noncopyable
    seed_rng(seed_rng const&);
    seed_rng& operator=(seed_rng const&);
};

// almost a copy of boost::generator_iterator
// but default constructor sets m_g to NULL
template <class Generator>
class generator_iterator
  : public iterator_facade<
        generator_iterator<Generator>
      , typename Generator::result_type
      , single_pass_traversal_tag
      , typename Generator::result_type const&
    >
{
    typedef iterator_facade<
        generator_iterator<Generator>
      , typename Generator::result_type
      , single_pass_traversal_tag
      , typename Generator::result_type const&
    > super_t;

 public:
    generator_iterator() : m_g(NULL), m_value(0) {}
    generator_iterator(Generator* g) : m_g(g), m_value((*m_g)()) {}

    void increment()
    {
        m_value = (*m_g)();
    }

    const typename Generator::result_type&
    dereference() const
    {
        return m_value;
    }

    bool equal(generator_iterator const& y) const
    {
        return this->m_g == y.m_g && this->m_value == y.m_value;
    }

 private:
    Generator* m_g;
    typename Generator::result_type m_value;
};

// seed() seeds a random number generator with good seed values

template <typename UniformRandomNumberGenerator>
inline void seed(UniformRandomNumberGenerator& rng)
{
    seed_rng seed_gen;
    generator_iterator<seed_rng> begin(&seed_gen);
    generator_iterator<seed_rng> end;
    rng.seed(begin, end);
}

// random_device does not / can not be seeded
template <>
inline void seed<boost::random::random_device>(boost::random::random_device&) {}

// random_device does not / can not be seeded
template <>
inline void seed<seed_rng>(seed_rng&) {}

}}} //namespace boost::uuids::detail

#if defined(_MSC_VER)
#pragma warning(pop) // Restore warnings to previous state.
#endif

#endif
