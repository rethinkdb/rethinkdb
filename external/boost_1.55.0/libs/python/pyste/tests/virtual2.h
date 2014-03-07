/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */

namespace virtual2 {

struct A
{
    virtual int f() { return 0; }
    virtual int f1() { return 10; }
    virtual A* make_new() { return new A; }
};

struct B: A
{
    virtual int f() { return 1; }
    virtual int f2() { return 20; }
    virtual A* make_new() { return new B; }
};

inline int call_fs(A*a)
{
    int r = a->f1();
    B* b = dynamic_cast<B*>(a);
    return r + b->f2();
}
    
inline int call_f(A* a)
{
    return a->f();
}
}
