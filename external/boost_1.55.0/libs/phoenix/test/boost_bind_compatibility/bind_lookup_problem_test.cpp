/*==============================================================================
    Copyright (c) 2005 Markus Schoepflin
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/bind.hpp>

template<class T> void value();

void f0() { }
void f1(int) { }
void f2(int, int) { }
void f3(int, int, int) { }
void f4(int, int, int, int) { }
void f5(int, int, int, int, int) { }
void f6(int, int, int, int, int, int) { }
void f7(int, int, int, int, int, int, int) { }
void f8(int, int, int, int, int, int, int, int) { }
void f9(int, int, int, int, int, int, int, int, int) { }

int main()
{
    using boost::phoenix::bind;
    
    bind(f0);
    bind(f1, 0);
    bind(f2, 0, 0);
    bind(f3, 0, 0, 0);
    bind(f4, 0, 0, 0, 0);
    bind(f5, 0, 0, 0, 0, 0);
    bind(f6, 0, 0, 0, 0, 0, 0);
    bind(f7, 0, 0, 0, 0, 0, 0, 0);
    bind(f8, 0, 0, 0, 0, 0, 0, 0, 0);
    bind(f9, 0, 0, 0, 0, 0, 0, 0, 0, 0);

  return 0;
}
