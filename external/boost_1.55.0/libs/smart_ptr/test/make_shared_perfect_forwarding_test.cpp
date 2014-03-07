// make_shared_perfect_forwarding_test.cpp - a test of make_shared
//   perfect forwarding of constructor arguments when using a C++0x
//   compiler.
//
// Copyright 2009 Frank Mori Hess
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/detail/lightweight_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#if defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

int main()
{
    return 0;
}

#else // !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

class myarg
{
public:
    myarg()
    {}
private:
    myarg(myarg && other)
    {}
    myarg& operator=(myarg && other)
    {
        return *this;
    }
    myarg(const myarg & other)
    {}
    myarg& operator=(const myarg & other)
    {
        return *this;
    }
};

class X
{
public:
    enum constructor_id
    {
        move_constructor,
        const_ref_constructor,
        ref_constructor
    };

    X(myarg &&arg): constructed_by_(move_constructor)
    {}
    X(const myarg &arg): constructed_by_(const_ref_constructor)
    {}
    X(myarg &arg): constructed_by_(ref_constructor)
    {}

    constructor_id constructed_by_;
};

struct Y
{
    Y(int &value): ref(value)
    {}
    int &ref;
};

int main()
{
    {
        myarg a;
        boost::shared_ptr< X > x = boost::make_shared< X >(a);
        BOOST_TEST( x->constructed_by_ == X::ref_constructor);
    }
    {
        const myarg ca;
        boost::shared_ptr< X > x = boost::make_shared< X >(ca);
        BOOST_TEST( x->constructed_by_ == X::const_ref_constructor);
    }
    {
        boost::shared_ptr< X > x = boost::make_shared< X >(myarg());
        BOOST_TEST( x->constructed_by_ == X::move_constructor);
    }
    {
        int value = 1;
        boost::shared_ptr< Y > y = boost::make_shared< Y >(value);
        BOOST_TEST( y->ref == 1 && value == y->ref );
        ++y->ref;
        BOOST_TEST( value == y->ref );
    }

    return boost::report_errors();
}

#endif // !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )
