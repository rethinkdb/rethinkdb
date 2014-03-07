/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */
#ifndef WRAPPER_TEST_WRAPPERS
#define WRAPPER_TEST_WRAPPERS 

#include <vector>
#include <boost/python.hpp>
#include "wrappertest.h"

using namespace boost::python;

template <class T>
list VectorToList(const std::vector<T> & v)
{
    list res;
    typename std::vector<T>::const_iterator it;
    for(it = v.begin(); it != v.end(); ++it){
        res.append(*it);
    }
    Py_XINCREF(res.ptr());
    return res;
}

inline list RangeWrapper(int count){
    return VectorToList(wrappertest::Range(count));
}

inline int f_wrapper(wrappertest::A*) { return 10; } 

#endif
