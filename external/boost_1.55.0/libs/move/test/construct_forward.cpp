//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright David Abrahams, Vicente Botet, Ion Gaztanaga 2009-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/detail/config_begin.hpp>
#include <boost/move/utility.hpp>
#include <boost/utility/enable_if.hpp>
#include "../example/movable.hpp"
#include "../example/copymovable.hpp"
#include <cstdio>

class non_movable
{
   public:
   non_movable()
   {}
};

template<class MaybeRvalue>
void catch_test(BOOST_RV_REF(MaybeRvalue) x
               #ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
               ,typename ::boost::enable_if< ::boost::has_move_emulation_enabled<MaybeRvalue> >::type* = 0
               #endif   //BOOST_NO_CXX11_RVALUE_REFERENCES
               )
{  (void)x;}

template<class MaybeRvalue>
void catch_test(BOOST_COPY_ASSIGN_REF(MaybeRvalue) x
               #ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
               ,typename ::boost::enable_if< ::boost::has_move_emulation_enabled<MaybeRvalue> >::type* = 0
               #endif   //BOOST_NO_CXX11_RVALUE_REFERENCES
               )

{  (void)x;}

template<class MaybeRvalue>
void catch_test(MaybeRvalue &x
               #ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
               ,typename ::boost::enable_if< ::boost::has_move_emulation_enabled<MaybeRvalue> >::type* = 0
               #endif   //BOOST_NO_CXX11_RVALUE_REFERENCES
               )
{  (void)x;}

               #ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
template<class MaybeRvalue>
void catch_test(const MaybeRvalue& x
               ,typename ::boost::disable_if< ::boost::has_move_emulation_enabled<MaybeRvalue> >::type* = 0
               )
{  (void)x;}
               #endif   //BOOST_NO_CXX11_RVALUE_REFERENCES

movable create_movable()
{  return movable(); }

copy_movable create_copy_movable()
{  return copy_movable(); }

non_movable create_non_movable()
{  return non_movable(); }


void catch_test()
{
   movable m;
   const movable constm;
   catch_test<movable>(boost::move(m));
   #ifdef BOOST_CATCH_CONST_RLVALUE
   catch_test<movable>(create_movable());
   #endif
   catch_test<movable>(m);
   catch_test<movable>(constm);
   copy_movable cm;
   const copy_movable constcm;
   catch_test<copy_movable>(boost::move(cm));
   catch_test<copy_movable>(create_copy_movable());
   catch_test<copy_movable>(cm);
   catch_test<copy_movable>(constcm);
   non_movable nm;
   const non_movable constnm;
   catch_test<non_movable>(boost::move(nm));
   catch_test<non_movable>(create_non_movable());
   catch_test<non_movable>(nm);
   catch_test<non_movable>(constnm);
}

template<class MaybeMovableOnly, class MaybeRvalue>
void function_construct(BOOST_FWD_REF(MaybeRvalue) x)
{
   //Moves in case Convertible is boost::rv<movable> copies otherwise
   //For C++0x perfect forwarding
   MaybeMovableOnly m(boost::forward<MaybeRvalue>(x));
}

void forward_test()
{
   movable m;
   function_construct<movable>(boost::move(m));
//   non_movable nm;
//   function_construct<non_movable>(boost::move(nm));
//   const non_movable cnm;
//   function_construct<non_movable>(cnm);
}

int main()
{
   catch_test();
   forward_test();
   return 0;
}

#include <boost/move/detail/config_end.hpp>
