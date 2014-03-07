/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */

#ifndef SMART_PTR_H
#define SMART_PTR_H


#include <memory>
#include <boost/shared_ptr.hpp>

namespace smart_ptr {
    
struct C
{
    int value;
};

inline boost::shared_ptr<C> NewC() { return boost::shared_ptr<C>( new C() ); }

struct D
{
    boost::shared_ptr<C> Get() { return ptr; }
    void Set( boost::shared_ptr<C> c ) { ptr = c; }
private:    
    boost::shared_ptr<C> ptr;
};

inline std::auto_ptr<D> NewD() { return std::auto_ptr<D>( new D() ); }


// test an abstract class
struct A
{
    virtual int f() = 0;
};

struct B: A
{
    virtual int f(){ return 1; }
};

inline boost::shared_ptr<A> NewA() { return boost::shared_ptr<A>(new B()); }
inline int GetA(boost::shared_ptr<A> a) { return a->f(); }

}

#endif
