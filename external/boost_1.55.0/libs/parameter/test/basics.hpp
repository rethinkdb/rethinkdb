// Copyright David Abrahams, Daniel Wallin 2005. Use, modification and 
// distribution is subject to the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BASICS_050424_HPP
#define BASICS_050424_HPP

#include <boost/parameter/keyword.hpp>
#include <boost/parameter/parameters.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/assert.hpp>
#include <boost/mpl/assert.hpp>
#include <cstring>
#include <boost/detail/lightweight_test.hpp>

namespace test {

BOOST_PARAMETER_KEYWORD(tag, name)
BOOST_PARAMETER_KEYWORD(tag, value)
BOOST_PARAMETER_KEYWORD(tag, index)
BOOST_PARAMETER_KEYWORD(tag, tester)

using namespace boost::parameter;

struct f_parameters // vc6 is happier with inheritance than with a typedef
  : parameters<
        tag::tester
      , tag::name
      , tag::value
      , tag::index
    >
{};

inline double value_default()
{
    return 666.222;
}

template <class T>
inline bool equal(T const& x, T const& y)
{
    return x == y;
}

inline bool equal(char const* s1, char const* s2)
{
    using namespace std;
    return !strcmp(s1,s2);
}

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
inline bool equal(char* s1, char* s2)
{
    using namespace std;
    return !strcmp(s1,s2);
}
#endif 

template <class Name, class Value, class Index>
struct values_t
{
    values_t(Name const& n, Value const& v, Index const& i)
      : n(n), v(v), i(i)
    {}

    template <class Name_, class Value_, class Index_>
    void operator()(Name_ const& n_, Value_ const& v_, Index_ const& i_) const
    {

        // Only VC and its emulators fail this; they seem to have
        // problems with deducing the constness of string literal
        // arrays.
#if defined(_MSC_VER)                                                   \
        && (BOOST_WORKAROUND(BOOST_INTEL_CXX_VERSION, <= 700)           \
            || BOOST_WORKAROUND(BOOST_MSVC, < 1310))                    \
            || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
# else 
        BOOST_MPL_ASSERT((boost::is_same<Index,Index_>));
        BOOST_MPL_ASSERT((boost::is_same<Value,Value_>));
        BOOST_MPL_ASSERT((boost::is_same<Name,Name_>));
#endif
        BOOST_TEST(equal(n, n_));
        BOOST_TEST(equal(v, v_));
        BOOST_TEST(equal(i, i_));
    }

    Name const& n;
    Value const& v;
    Index const& i;
};

template <class Name, class Value, class Index>
inline values_t<Name,Value,Index>
values(Name const& n, Value const& v, Index const& i)
{
    return values_t<Name,Value,Index>(n,v,i);
}

} // namespace test

// GCC2 has a problem with char (&)[] deduction, so we'll cast string
// literals there.
#undef S
#if BOOST_WORKAROUND(__GNUC__, == 2) || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
# define S(s) (char const*)s
#else
# define S(s) s
#endif

#endif // BASICS_050424_HPP

