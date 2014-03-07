//  (C) Copyright Mathias Gaunard 2009. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_SFINAE_EXPR
//  TITLE:         SFINAE for expressions
//  DESCRIPTION:   SFINAE for expressions not supported.

namespace boost_no_sfinae_expr
{

template<typename T>
struct has_foo
{
    typedef char NotFound;
    struct Found { char x[2]; };
                                                                        
    template<int> struct dummy {};
    
    template<class X> static Found test(dummy< sizeof((*(X*)0).foo(), 0) >*);
    template<class X> static NotFound test( ... );
       
    static const bool value  = (sizeof(Found) == sizeof(test<T>(0)));
};

struct test1 {};
struct test2 { void foo(); };

int test()
{
   return has_foo<test1>::value || !has_foo<test2>::value;
}

} // namespace boost_no_sfinae_expr
