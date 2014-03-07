/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */ 
#include <vector>
#include <string>

namespace abstract {
    
struct A {
    virtual ~A() {}
    virtual std::string f()=0;
};

struct B: A {
    std::string f() { return "B::f"; }
};

std::string call(A* a) { return a->f(); }
  
}
