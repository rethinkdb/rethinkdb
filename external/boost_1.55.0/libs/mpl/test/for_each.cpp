
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: for_each.cpp 66257 2010-10-29 21:36:38Z eric_niebler $
// $Date: 2010-10-29 14:36:38 -0700 (Fri, 29 Oct 2010) $
// $Revision: 66257 $

#include <boost/mpl/for_each.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/bind.hpp>

#include <vector>
#include <iostream>
#include <algorithm>
#include <typeinfo>
#include <cassert>

namespace mpl = boost::mpl;

struct type_printer
{
    type_printer(std::ostream& s) : f_stream(&s) {}
    template< typename U > void operator()(mpl::identity<U>)
    {
        *f_stream << typeid(U).name() << '\n';
    }

 private:
    std::ostream* f_stream;
};

struct value_printer
{
    value_printer(std::ostream& s) : f_stream(&s) {}
    template< typename U > void operator()(U x)
    {
        *f_stream << x << '\n';
    }

 private:
    std::ostream* f_stream;
};

#ifdef __ICL
# pragma warning(disable:985)
#endif

void push_back(std::vector<int>* c, int i)
{
    c->push_back(i);
}

int main()
{
    typedef mpl::list<char,short,int,long,float,double> types;
    mpl::for_each< types,mpl::make_identity<mpl::_1> >(type_printer(std::cout));

    typedef mpl::range_c<int,0,10> numbers;
    std::vector<int> v;

    mpl::for_each<numbers>(
          boost::bind(&push_back, &v, _1)
        );

    mpl::for_each< numbers >(value_printer(std::cout));
    
    for (unsigned i = 0; i < v.size(); ++i)
        assert(v[i] == (int)i);

    return 0;
}
