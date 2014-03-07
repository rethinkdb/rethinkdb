/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */
#ifndef OPAQUE_H
#define OPAQUE_H

#include <iostream>

namespace opaque {

    
struct C { 
    C(int v): value(v) {}
    int value; 
};


inline C* new_C()
{
    return new C(10);
}

inline C* new_C_zero()
{
    return new C(0);
}

inline int get(C* c)
{
    return c->value;
}

struct D { 
    D(double v): value(v) {}
    double value; 
};

struct A
{
    D* new_handle()
    {
        return new D(3.0); 
    }

    double get(D* d)
    {
        return d->value;
    }
    
    int f(int x=0) { return x; }
};

}

#endif
