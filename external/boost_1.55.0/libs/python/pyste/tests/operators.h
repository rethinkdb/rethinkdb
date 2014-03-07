/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */
#ifndef OPERATORS_H
#define OPERATORS_H


namespace operators {

struct C
{
    static double x;
    double value;

    const C operator+(const C other) const
    {
        C c;
        c.value = value + other.value;
        return c;
    }
    operator int() const
    {
        return (int)value;    
    } 
    
    double operator()()
    {
        return C::x;
    }

    double operator()(double other)
    {
        return C::x + other;
    }
    
    operator const char*() { return "C"; }
};

inline const C operator*(const C& lhs, const C& rhs)
{
    C c;
    c.value = lhs.value * rhs.value;
    return c;
}


}


#endif
