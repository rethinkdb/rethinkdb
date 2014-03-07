// Copyright David Abrahams 2004. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/pointee.hpp>
#include <boost/type_traits/add_const.hpp>
#include "static_assert_same.hpp"
#include <memory>
#include <list>

template <class T, class Ref>
struct proxy_ptr
{
    typedef T element_type;
    struct proxy
    {
        operator Ref() const;
    };
    proxy operator*() const;
};

template <class T>
struct proxy_ref_ptr : proxy_ptr<T,T&>
{
};

template <class T>
struct proxy_value_ptr : proxy_ptr<T,T>
{
    typedef typename boost::add_const<T>::type element_type;
};

struct X {
    template <class T> X(T const&);
    template <class T> operator T&() const;
};

BOOST_TT_BROKEN_COMPILER_SPEC(X)
    
int main()
{
    STATIC_ASSERT_SAME(boost::pointee<proxy_ref_ptr<int> >::type, int);
    STATIC_ASSERT_SAME(boost::pointee<proxy_ref_ptr<X> >::type, X);

    STATIC_ASSERT_SAME(boost::pointee<proxy_ref_ptr<int const> >::type, int const);
    STATIC_ASSERT_SAME(boost::pointee<proxy_ref_ptr<X const> >::type, X const);
    
    STATIC_ASSERT_SAME(boost::pointee<proxy_value_ptr<int> >::type, int const);
    STATIC_ASSERT_SAME(boost::pointee<proxy_value_ptr<X> >::type, X const);
    
    STATIC_ASSERT_SAME(boost::pointee<proxy_value_ptr<int const> >::type, int const);
    STATIC_ASSERT_SAME(boost::pointee<proxy_value_ptr<X const> >::type, X const);

    STATIC_ASSERT_SAME(boost::pointee<int*>::type, int);
    STATIC_ASSERT_SAME(boost::pointee<int const*>::type, int const);
    
    STATIC_ASSERT_SAME(boost::pointee<X*>::type, X);
    STATIC_ASSERT_SAME(boost::pointee<X const*>::type, X const);

    STATIC_ASSERT_SAME(boost::pointee<std::auto_ptr<int> >::type, int);
    STATIC_ASSERT_SAME(boost::pointee<std::auto_ptr<X> >::type, X);
    
    STATIC_ASSERT_SAME(boost::pointee<std::auto_ptr<int const> >::type, int const);
    STATIC_ASSERT_SAME(boost::pointee<std::auto_ptr<X const> >::type, X const);

    STATIC_ASSERT_SAME(boost::pointee<std::list<int>::iterator >::type, int);
    STATIC_ASSERT_SAME(boost::pointee<std::list<X>::iterator >::type, X);
    
    STATIC_ASSERT_SAME(boost::pointee<std::list<int>::const_iterator >::type, int const);
    STATIC_ASSERT_SAME(boost::pointee<std::list<X>::const_iterator >::type, X const);
    return 0;
}
