/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */
#ifndef ENUMS_H
#define ENUMS_H

namespace enums {
    
enum color { red, blue };

struct X
{
    enum choices
    {
        good = 1,
        bad = 2
    };

    int set(choices c)
    {
        return (int)c;
    }
};

enum {
    x = 0,
    y = 1
};

}

#endif
