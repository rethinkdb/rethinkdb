/* concepts.hpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: concepts.hpp 72951 2011-07-07 04:57:37Z steven_watanabe $
 *
 */

#ifndef BOOST_RANDOM_TEST_CONCEPTS_HPP
#define BOOST_RANDOM_TEST_CONCEPTS_HPP

#include <boost/config.hpp>

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4100)
#endif

#include <boost/concept_check.hpp>

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#include <boost/concept_archetype.hpp>
#include <boost/concept/requires.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <istream>
#include <ostream>

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4510)
#pragma warning(disable:4610)
#endif

namespace boost {
namespace random {
namespace test {

template<class Base = null_archetype<> >
struct seed_seq_archetype : Base
{
    template<class Iter>
    BOOST_CONCEPT_REQUIRES(
        ((Mutable_RandomAccessIterator<Iter>))
        ((UnsignedInteger<typename Mutable_RandomAccessIterator<Iter>::value_type>)),
    (void))
    generate(Iter, Iter) {}
};

template<class R = unsigned, class Base = null_archetype<> >
struct uniform_random_number_generator_archetype : Base
{
    typedef R result_type;
    static R min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
    static R max BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
    R operator()() { return 0; }
};

template<class SSeq>
struct SeedSeq
{
public:
    BOOST_CONCEPT_USAGE(SeedSeq)
    {
        q.generate(rb, re);
    }
private:
    SSeq q;
    mutable_random_access_iterator_archetype<boost::uint32_t> rb, re;
};

template<class T>
struct Streamable
{
public:
    BOOST_CONCEPT_USAGE(Streamable)
    {
        os << x;
        is >> v;
        wos << x;
        wis >> v;
    }
private:
    const T x;
    T v;

    std::istream is;
    std::ostream os;
    std::wistream wis;
    std::wostream wos;
};

// Type deduction will fail unless the arguments have the same type.
template <typename T>
void same_type(T const&, T const&) {}

template <class E>
struct RandomNumberEngine :
    DefaultConstructible<E>,
    CopyConstructible<E>,
    Assignable<E>,
    EqualityComparable<E>,
    Streamable<E>
{
public:
    typedef typename E::result_type result_type;

    // relaxed from the standard
    BOOST_MPL_ASSERT((boost::is_arithmetic<result_type>));

    // backwards compatibility check
    BOOST_STATIC_ASSERT(!E::has_fixed_range);

    // a generator can be used to seed another generator (extension)
    BOOST_CONCEPT_ASSERT((SeedSeq<E>));

    BOOST_CONCEPT_USAGE(RandomNumberEngine)
    {
        same_type(e(), result_type());
        same_type((E::min)(), result_type());
        same_type((E::max)(), result_type());

        (void)E();
        (void)E(s);
        (void)E(q);

        e.seed();
        e.seed(s);
        e.seed(q);

        e.discard(z);

        // extension
        (void)E(sb, se);
        e.seed(sb, se);
    }
    
private:
    E e;
    E v;
    const E x;
    seed_seq_archetype<> q;
    typename detail::seed_type<result_type>::type s;
    unsigned long long z;
    
    input_iterator_archetype<boost::uint32_t> sb, se;
};

template<class D>
struct RandomNumberDistribution :
    DefaultConstructible<D>,
    CopyConstructible<D>,
    Assignable<D>,
    EqualityComparable<D>,
    Streamable<D>
{
public:
    typedef typename D::result_type result_type;
    typedef typename D::param_type param_type;
    // backwards compatibility
    typedef typename D::input_type input_type;

    typedef param_type P;

    BOOST_CONCEPT_ASSERT((DefaultConstructible<P>));
    BOOST_CONCEPT_ASSERT((CopyConstructible<P>));
    BOOST_CONCEPT_ASSERT((Assignable<P>));
    BOOST_CONCEPT_ASSERT((EqualityComparable<P>));
    BOOST_CONCEPT_ASSERT((Streamable<P>));

    BOOST_MPL_ASSERT((boost::is_same<typename P::distribution_type, D>));

    BOOST_CONCEPT_USAGE(RandomNumberDistribution)
    {
        (void)D(p);
        d.reset();
        same_type(x.param(), p);
        d.param(p);
        same_type(d(g), result_type());
        same_type(d(g, p), result_type());
        same_type((x.min)(), result_type());
        same_type((x.max)(), result_type());
    }

private:
    D d;
    const D x;
    const P p;
    uniform_random_number_generator_archetype<> g;
};

}
}
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif
