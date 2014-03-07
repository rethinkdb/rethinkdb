/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */

struct Color
{
    Color(int r_ = 0, int g_ = 0, int b_ = 0):
        r(r_), g(g_), b(b_)
    {}        
    Color( const Color &c):
        r(c.r), g(c.g), b(c.b)
    {}
    int r;
    int g;
    int b;
};

extern const Color black;
extern const Color red;
extern const Color green;
extern const Color blue;
extern Color in_use;
