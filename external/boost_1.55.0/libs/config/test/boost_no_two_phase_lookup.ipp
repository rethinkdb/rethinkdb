//  (C) Copyright Alisdair Meredith 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_TWO_PHASE_NAME_LOOKUP
//  TITLE:         Two phase name lookup
//  DESCRIPTION:   If the compiler does not perform two phase name lookup

namespace boost_no_two_phase_name_lookup {

template< class T >
struct base {
    int call() {
        return 1;
    }
};

int call() {
    return 0;
}

template< class T >
struct derived : base< T > {
    int call_test() {
        return call();
    }
};

int test()
{
    derived< int > d;
    return d.call_test();
}

}



