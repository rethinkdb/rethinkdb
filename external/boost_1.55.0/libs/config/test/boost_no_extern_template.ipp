//  (C) Copyright Beman Dawes 2008

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_EXTERN_TEMPLATE
//  TITLE:         C++0x extern template unavailable
//  DESCRIPTION:   The compiler does not support C++0x extern template

namespace boost_no_cxx11_extern_template {

template<class T, class U> void f(T const* p, U const* q)
{
   p = q;
}

template <class T>
class must_not_compile
{
public:
   void f(T const* p, int const* q);
};

template <class T>
void must_not_compile<T>::f(T const* p, int const* q)
{
   p = q;
}

extern template void f<>(int const*, float const*);
extern template class must_not_compile<int>;

int test()
{
  return 0;
}

}
