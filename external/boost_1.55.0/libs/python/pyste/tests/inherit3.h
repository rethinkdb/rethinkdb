/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */

namespace inherit3 {
    
struct A
{
    A() { x = 0; }
    struct X { int y; };
    int x;
    virtual int foo() { return 0; }
    virtual int foo(int x) { return x; }
    A operator+(A o) const
    {
        A r;
        r.x = o.x + x;
        return r;
    }
    enum E { i, j };

};

struct B: A
{
    B() { x = 0; }
    struct X { int y; };
    int x;
    int foo() { return 1; }
    A operator+(A o) const
    {
        A r;
        r.x = o.x + x;
        return r;
    }     
    enum E { i, j };

}; 

struct C: A
{
};  

}
