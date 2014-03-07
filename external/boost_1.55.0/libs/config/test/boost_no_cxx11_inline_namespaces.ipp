//  (C) Copyright Andrey Semashev 2013

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_INLINE_NAMESPACES
//  TITLE:         C++11 inline namespaces.
//  DESCRIPTION:   The compiler does not support C++11 inline namespaces.

namespace boost_no_cxx11_inline_namespaces {

inline namespace my_ns {

int data = 0;

} // namespace my_ns

int test()
{
    data = 1;
    if (&data == &my_ns::data)
        return 0;
    else
        return 1;
}

}
