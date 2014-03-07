/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/home/phoenix/detail/type_deduction.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <complex>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>

namespace boost
{
    BOOST_UNARY_RESULT_OF(-x, result_of_negate);
    BOOST_UNARY_RESULT_OF(+x, result_of_posit);
    BOOST_UNARY_RESULT_OF(!x, result_of_logical_not);
    BOOST_UNARY_RESULT_OF(~x, result_of_invert);
    BOOST_UNARY_RESULT_OF(&x, result_of_reference);
    BOOST_UNARY_RESULT_OF(*x, result_of_dereference);

    BOOST_UNARY_RESULT_OF(++x, result_of_pre_increment);
    BOOST_UNARY_RESULT_OF(--x, result_of_pre_decrement);
    BOOST_UNARY_RESULT_OF(x++, result_of_post_increment);
    BOOST_UNARY_RESULT_OF(x--, result_of_post_decrement);

    BOOST_BINARY_RESULT_OF(x = y, result_of_assign);
    BOOST_ASYMMETRIC_BINARY_RESULT_OF(x[y], result_of_index);

    BOOST_BINARY_RESULT_OF(x += y, result_of_plus_assign);
    BOOST_BINARY_RESULT_OF(x -= y, result_of_minus_assign);
    BOOST_BINARY_RESULT_OF(x *= y, result_of_multiplies_assign);
    BOOST_BINARY_RESULT_OF(x /= y, result_of_divides_assign);
    BOOST_BINARY_RESULT_OF(x %= y, result_of_modulus_assign);

    BOOST_BINARY_RESULT_OF(x &= y, result_of_and_assign);
    BOOST_BINARY_RESULT_OF(x |= y, result_of_or_assign);
    BOOST_BINARY_RESULT_OF(x ^= y, result_of_xor_assign);
    BOOST_BINARY_RESULT_OF(x <<= y, result_of_shift_left_assign);
    BOOST_BINARY_RESULT_OF(x >>= y, result_of_shift_right_assign);

    BOOST_BINARY_RESULT_OF(x + y, result_of_plus);
    BOOST_BINARY_RESULT_OF(x - y, result_of_minus);
    BOOST_BINARY_RESULT_OF(x * y, result_of_multiplies);
    BOOST_BINARY_RESULT_OF(x / y, result_of_divides);
    BOOST_BINARY_RESULT_OF(x % y, result_of_modulus);

    BOOST_BINARY_RESULT_OF(x & y, result_of_and);
    BOOST_BINARY_RESULT_OF(x | y, result_of_or);
    BOOST_BINARY_RESULT_OF(x ^ y, result_of_xor);
    BOOST_BINARY_RESULT_OF(x << y, result_of_shift_left);
    BOOST_BINARY_RESULT_OF(x >> y, result_of_shift_right);

    BOOST_BINARY_RESULT_OF(x == y, result_of_equal_to);
    BOOST_BINARY_RESULT_OF(x != y, result_of_not_equal_to);
    BOOST_BINARY_RESULT_OF(x < y, result_of_less);
    BOOST_BINARY_RESULT_OF(x <= y, result_of_less_equal);
    BOOST_BINARY_RESULT_OF(x > y, result_of_greater);
    BOOST_BINARY_RESULT_OF(x >= y, result_of_greater_equal);

    BOOST_BINARY_RESULT_OF(x && y, result_of_logical_and);
    BOOST_BINARY_RESULT_OF(x || y, result_of_logical_or);
    BOOST_BINARY_RESULT_OF(true ? x : y, result_of_if_else);
}

using namespace boost;
using namespace std;

struct X {};
X operator+(X, int);

struct Y {};
Y* operator+(Y, int);

struct Z {};
Z const* operator+(Z const&, int);
Z& operator+(Z&, int);
bool operator==(Z, Z);
bool operator==(Z, int);

struct W {};
Z operator+(W, int);
bool operator==(W, Z);

int
main()
{
    //  ASSIGN
    {
        typedef result_of_assign<int, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int&>::value));
    }

    {
        typedef result_of_assign<int*, int*>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int*&>::value));
    }

    //  PLUS
    {
        typedef result_of_plus<int, double>::type result;
        BOOST_STATIC_ASSERT((is_same<result, double>::value));
    }
    {
        typedef result_of_plus<double, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, double>::value));
    }
    {
        typedef result_of_plus<int, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int>::value));
    }
    {
        typedef result_of_plus<float, short>::type result;
        BOOST_STATIC_ASSERT((is_same<result, float>::value));
    }
    {
        typedef result_of_plus<char, short>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int>::value));
    }
    {
        typedef result_of_plus<long, short>::type result;
        BOOST_STATIC_ASSERT((is_same<result, long>::value));
    }
    {
        typedef result_of_plus<long, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, long>::value));
    }
    {
        typedef result_of_plus<X, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X>::value));
    }
    {
        typedef result_of_plus<Y, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, Y*>::value));
    }
    {
        typedef result_of_plus<Z, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, Z&>::value));
    }
    {
        typedef result_of_plus<Z const, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, Z const*>::value));
    }
    {
        typedef result_of_plus<complex<double>, double>::type result;
        BOOST_STATIC_ASSERT((is_same<result, complex<double> >::value));
    }
    {
        typedef result_of_plus<double, complex<double> >::type result;
        BOOST_STATIC_ASSERT((is_same<result, complex<double> >::value));
    }
    {
        typedef result_of_plus<int*, size_t>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int*>::value));
    }

    //  INDEX
    {
        typedef result_of_index<int(&)[3], int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int&>::value));
    }
    {
        typedef result_of_index<X(&)[3], int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X&>::value));
    }
    {
        typedef result_of_index<X const(&)[3], int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X const&>::value));
    }
    {
        typedef result_of_index<X*, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X&>::value));
    }
    {
        typedef result_of_index<X const*, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X const&>::value));
    }
    {
        typedef result_of_index<vector<int>, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, vector<int>::reference>::value));
    }
    {
        typedef result_of_index<vector<int> const, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int>::value));
    }
    {
        typedef result_of_index<vector<X> const, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, vector<X>::const_reference>::value));
    }
    {
        typedef result_of_index<vector<X>, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, vector<X>::reference>::value));
    }
    {
        typedef result_of_index<string, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, string::reference>::value));
    }
    {
        typedef result_of_index<vector<int>::iterator, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, vector<int>::iterator::reference>::value));
    }
    {
        typedef result_of_index<vector<int>::const_iterator, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int>::value));
    }
    {
        typedef result_of_index<vector<X>::const_iterator, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, vector<X>::const_iterator::reference>::value));
    }
    {
        typedef result_of_index<map<char, X>, char>::type result;
        BOOST_STATIC_ASSERT((is_same<result, map<char, X>::mapped_type>::value));
    }

    //  PLUS ASSIGN
    {
        typedef result_of_plus_assign<int, char>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int&>::value));
    }
    {
        typedef result_of_plus_assign<double, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, double&>::value));
    }
    {
        typedef result_of_plus_assign<complex<double>, double>::type result;
        BOOST_STATIC_ASSERT((is_same<result, complex<double>&>::value));
    }

    //  SHIFT LEFT
    {
        typedef result_of_shift_left<int, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int>::value));
    }
    {
        typedef result_of_shift_left<short, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int>::value));
    }
    {
        typedef result_of_shift_left<ostream, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, ostream&>::value));
    }

    //  EQUAL
    {
        typedef result_of_equal_to<int, double>::type result;
        BOOST_STATIC_ASSERT((is_same<result, bool>::value));
    }
    {
        typedef result_of_equal_to<double, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, bool>::value));
    }
    {
        typedef result_of_equal_to<int, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, bool>::value));
    }
    {
        typedef result_of_equal_to<float, short>::type result;
        BOOST_STATIC_ASSERT((is_same<result, bool>::value));
    }
    {
        typedef result_of_equal_to<char, short>::type result;
        BOOST_STATIC_ASSERT((is_same<result, bool>::value));
    }
    {
        typedef result_of_equal_to<Z, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, bool>::value));
    }
    {
        typedef result_of_equal_to<Z, Z>::type result;
        BOOST_STATIC_ASSERT((is_same<result, bool>::value));
    }
    {
        typedef result_of_equal_to<W, Z>::type result;
        BOOST_STATIC_ASSERT((is_same<result, bool>::value));
    }

    //  MINUS (pointers)
    {
        typedef result_of_minus<X*, X*>::type result;
        BOOST_STATIC_ASSERT((is_same<result, std::ptrdiff_t>::value));
    }

    //  DEREFERENCE
    {
        typedef result_of_dereference<X*>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X&>::value));
    }
    {
        typedef result_of_dereference<vector<X>::iterator>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X&>::value));
    }
    {
        typedef result_of_dereference<shared_ptr<X> >::type result;
        BOOST_STATIC_ASSERT((is_same<result, X&>::value));
    }

    //  ADDRESS OF
    {
        typedef result_of_reference<X>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X*>::value));
    }
    {
        typedef result_of_reference<X const>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X const*>::value));
    }

    //  PRE INCREMENT
    {
        typedef result_of_pre_increment<int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int&>::value));
    }

    //  POST INCREMENT
    {
        typedef result_of_post_increment<int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int>::value));
    }

    //  IF-ELSE-EXPRESSION ( c ? a : b )
    {
        typedef result_of_if_else<int, char>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int>::value));
    }
    {
        typedef result_of_if_else<int, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int&>::value));
    }
    {
        typedef result_of_if_else<int const, int const>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int>::value));
    }
    {
        typedef result_of_if_else<X, X>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X&>::value));
    }
    {
        typedef result_of_if_else<X const&, X const&>::type result;
        BOOST_STATIC_ASSERT((is_same<result, X const&>::value));
    }

    //  DEDUCTION FAILURE
    {
        typedef result_of_plus<W, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, error_cant_deduce_type>::value));
    }
    
    // local_reference
    {
        using phoenix::detail::local_reference;
        typedef result_of_assign<local_reference<int>, int>::type result;
        BOOST_STATIC_ASSERT((is_same<result, int&>::value));
    }

    // local_reference
    {
        using phoenix::detail::local_reference;
        typedef result_of_pre_increment<local_reference<int> >::type result;
        BOOST_STATIC_ASSERT((is_same<result, int&>::value));
    }

    // local_reference
    {
        using phoenix::detail::local_reference;
        typedef result_of_if_else<local_reference<X const>, local_reference<X const> >::type result;
        BOOST_STATIC_ASSERT((is_same<result, X const&>::value));
    }

    return boost::report_errors();
}
