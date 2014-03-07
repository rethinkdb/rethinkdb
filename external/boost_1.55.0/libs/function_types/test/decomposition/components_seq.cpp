
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/type_traits/is_same.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/equal.hpp>

#include <boost/mpl/front.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/clear.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/pop_back.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/empty.hpp>

#include <boost/function_types/components.hpp>

namespace ft = boost::function_types;
namespace mpl = boost::mpl;
using boost::is_same;

class C;
typedef C (C::* mem_func_ptr)(int);
typedef ft::components<mem_func_ptr> c;


// front
BOOST_MPL_ASSERT(( is_same< ::boost::mpl::front<c>::type, C > ));

// back
BOOST_MPL_ASSERT(( is_same< ::boost::mpl::back<c>::type, int > ));

// at
BOOST_MPL_ASSERT(( is_same< ::boost::mpl::at_c<c,0>::type, C   > ));
BOOST_MPL_ASSERT(( is_same< ::boost::mpl::at_c<c,1>::type, C & > ));
BOOST_MPL_ASSERT(( is_same< ::boost::mpl::at_c<c,2>::type, int > ));

// begin/end
BOOST_MPL_ASSERT(( mpl::equal< c, mpl::vector<C,C &, int> > ));


// clear
BOOST_MPL_ASSERT(( mpl::equal< mpl::clear<c>, mpl::vector<> > ));

// push_front
BOOST_MPL_ASSERT(( mpl::equal< mpl::push_front<c,C>::type, mpl::vector<C,C,C &,int> > ));

// pop_front
BOOST_MPL_ASSERT(( mpl::equal< mpl::pop_front<c>::type, mpl::vector<C &,int> > ));

// push_back
BOOST_MPL_ASSERT(( mpl::equal< mpl::push_back<c,C>::type, mpl::vector<C,C &,int,C> > ));

// pop_back
BOOST_MPL_ASSERT(( mpl::equal< mpl::pop_back<c>::type, mpl::vector<C,C &> > ));


// size
BOOST_MPL_ASSERT_RELATION( ::boost::mpl::size<c>::value, ==, 3 );

// empty
BOOST_MPL_ASSERT_NOT(( mpl::empty<c> ));
BOOST_MPL_ASSERT(( mpl::empty< mpl::clear<c> > ));

