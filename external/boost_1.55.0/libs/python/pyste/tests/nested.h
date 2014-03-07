/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */ 

#ifndef NESTED_H
#define NESTED_H

namespace nested {
    
struct X
{
    struct Y 
    {
        int valueY;
        static int staticYValue; 
        struct Z
        {
            int valueZ;
        };
    };
    
    static int staticXValue;
    int valueX;
};

typedef X Root;

}

#endif
