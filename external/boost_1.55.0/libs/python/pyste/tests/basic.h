/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */
#ifndef BASIC_H
#define BASIC_H


#include <string>

namespace basic {
    
struct C
{    
    // test virtuallity
    C(): value(1), const_value(0) {}
    virtual int f(int x = 10)
    {
        return x*2;
    }    
    
    int foo(int x=1){
        return x+1;
    }

    const std::string& get_name() { return name; }
    void set_name(const std::string& name) { this->name = name; }
private:
    std::string name;

public:
    // test data members
    static int static_value;
    static const int const_static_value;
    
    int value;
    const int const_value;

    // test static functions
    static int mul(int x, int y) { return x*y; }
    static double mul(double x, double y) { return x*y; }

    static int square(int x=2) { return x*x; }
};

inline int call_f(C& c)
{
    return c.f();
}

inline int call_f(C& c, int x)
{
    return c.f(x);
} 

inline int get_static()
{
    return C::static_value;
}

inline int get_value(C& c)
{
    return c.value;
}

}

#endif
