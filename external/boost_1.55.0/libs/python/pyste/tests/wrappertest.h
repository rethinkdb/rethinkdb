/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */
#ifndef WRAPPER_TEST
#define WRAPPER_TEST


#include <vector>

namespace wrappertest {

inline std::vector<int> Range(int count)
{
    std::vector<int> v;
    v.reserve(count);
    for (int i = 0; i < count; ++i){
        v.push_back(i);
    }
    return v;
}


struct C
{
    C() {}

    std::vector<int> Mul(int value)
    {
        std::vector<int> res;
        res.reserve(value);
        std::vector<int>::const_iterator it;
        std::vector<int> v(Range(value));
        for (it = v.begin(); it != v.end(); ++it){
            res.push_back(*it * value);
        }
        return res;
    }
};


struct A
{
    virtual int f() { return 1; };
};

inline int call_foo(A* a){ return a->f(); }
} 
#endif

