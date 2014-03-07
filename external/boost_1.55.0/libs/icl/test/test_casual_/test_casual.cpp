/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::casual unit test

#define BOOST_ICL_TEST_CHRONO

#include <libs/icl/test/disable_test_warnings.hpp>

#include <string>
#include <vector>
#include <boost/mpl/list.hpp>
#include "../unit_test_unwarned.hpp"


// interval instance types
#include "../test_type_lists.hpp"
#include "../test_value_maker.hpp"

#include <boost/rational.hpp>

#include <boost/type_traits/is_same.hpp>

#include <boost/icl/gregorian.hpp>
#include <boost/icl/ptime.hpp>

#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/icl/interval.hpp>


namespace my
{

class Tag
{
public:
    Tag():_val(0){}
    Tag(int val):_val(val){}
    int val()const { return _val; }

    Tag& operator += (const Tag& rhs){ return *this; }
    Tag& operator -= (const Tag& rhs){ if(_val == rhs.val()) _val=0; return *this; }
    Tag& operator &= (const Tag& rhs){ if(_val != rhs.val()) _val=0; return *this; }

private:
    int _val;
};

bool operator == (const Tag& lhs, const Tag& rhs){ return lhs.val() == rhs.val(); }
bool operator <  (const Tag& lhs, const Tag& rhs){ return lhs.val()  < rhs.val(); }

template<class CharType, class CharTraits>
std::basic_ostream<CharType, CharTraits> &operator<<
  (std::basic_ostream<CharType, CharTraits> &stream, Tag const& value)
{
    return stream << value.val();
}

} // namespace my 

namespace boost{ namespace icl{
    template<> struct is_set<my::Tag>
    {
        typedef is_set type;
        BOOST_STATIC_CONSTANT(bool, value = true); 
    };
}} // namespace icl boost

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;


BOOST_AUTO_TEST_CASE(CacheSegments)
{
    using namespace my;

    typedef int T;
    typedef int U;
    typedef interval_map<T,U, partial_absorber> IntervalMapT;
    typedef interval_set<T>                     IntervalSetT;
    typedef IntervalMapT::interval_type         IntervalT;

    typedef interval_map<int, Tag> InsMapT;
    InsMapT imap;

    imap += make_pair(interval<int>::right_open( 0, 8),  Tag(1));
    imap += make_pair(interval<int>::right_open( 8,16),  Tag(2));
    imap += make_pair(interval<int>::right_open(16,18),  Tag(1));
    imap += make_pair(interval<int>::right_open(17,20),  Tag(2));
    cout << imap << endl;

    //imap -= make_pair(interval<int>::right_open( 4,20),  Tag(1));  //drop_if(1)
    imap &= make_pair(interval<int>::right_open( 4,20),  Tag(1));  //keep_if(1)
    cout << imap << endl;

    BOOST_CHECK_EQUAL(true, true);
}

BOOST_AUTO_TEST_CASE(casual)
{
    typedef int T;
    typedef int U;
    typedef interval_map<T,U, partial_absorber> IntervalMapT;
    typedef interval_set<T>                     IntervalSetT;
    typedef IntervalMapT::interval_type         IntervalT;


    BOOST_CHECK_EQUAL(true, true);
}


#include <boost/chrono.hpp>

typedef chrono::time_point<chrono::system_clock> time_point;
typedef icl::interval_map<time_point, int> IntervalMap;
typedef IntervalMap::interval_type Interval;

template<class D, class C>
void add_closed(icl::interval_map<D,std::set<C>>& im, D low, D up, C data)
{
    typedef typename icl::interval_map<D,std::set<C>> IntervalMap;
    typedef typename IntervalMap::interval_type Interval;
    std::set<C> single;
    single.insert(std::move(data));
    im += make_pair(Interval::closed(low, up), single);
}

void icl_and_chrono()
{
    typedef chrono::time_point<chrono::system_clock> time_point;
    typedef icl::interval_map<time_point, std::set<int> > IntervalMap;

    IntervalMap im;
    auto now = chrono::system_clock::now();

    add_closed(im, now, now + chrono::seconds(10), 42);
    add_closed(im, now + chrono::seconds(5), now + chrono::seconds(15), 24);
    cout << im << endl;
}

void icl_chrono_101()
{
  typedef chrono::time_point<chrono::system_clock> time_point;
  typedef icl::interval_map<time_point, int> IntervalMap;
  typedef IntervalMap::interval_type Interval;

  IntervalMap im;
  auto now = chrono::system_clock::now();
  im += make_pair(Interval::closed(now, now + chrono::seconds(10)), 42);
  icl::discrete_interval<time_point> itv2 
    = Interval::closed(now + chrono::seconds(5), now + chrono::seconds(15));
  im += make_pair(itv2, 24);

  cout << im << endl;
}

BOOST_AUTO_TEST_CASE(chrono_101)
{
    icl_and_chrono();
}


template <typename Domain, typename Codomain>
class interval_map2
{
public:
  void add_closed(Domain lower, Domain upper, Codomain data)
  {
    set_type set;
    set.insert(std::move(data));
    map_ += std::make_pair(interval_type::closed(lower, upper), set);
  } 

private:
  typedef std::set<Codomain> set_type;
  typedef boost::icl::discrete_interval<Domain> interval_type;
  typedef boost::icl::interval_map<Domain, set_type> map_type;

  map_type map_;
};

BOOST_AUTO_TEST_CASE(chrono_102)
{
  typedef chrono::time_point<chrono::system_clock> time_point;
  interval_map2<time_point, int> im;
  auto now = chrono::system_clock::now();
  im.add_closed(now, now + chrono::seconds(10), 42);

} 

//--------------------------------------
BOOST_AUTO_TEST_CASE(hudel)
{
    typedef int Time;
    icl::interval_set<Time> _badMonths;
    std::vector<int> badMonths;
    badMonths.push_back(2);
    badMonths.push_back(6);
    badMonths.push_back(9);

    for(int m : badMonths)
    {
        icl::interval<Time>::type month(m, m+2);
        _badMonths.insert(month);
    }

    Time timepoint = 7;
    if ( icl::contains(_badMonths, timepoint) ) 
	    std::cout <<  timepoint  << " intersects bad months" << std::endl;
    if ( icl::within(timepoint, _badMonths) ) 
	    std::cout <<  timepoint  << " intersects bad months" << std::endl;
    if ( icl::intersects(_badMonths, timepoint) ) 
	    std::cout <<  timepoint  << " intersects bad months" << std::endl;
    if ( _badMonths.find(timepoint) != _badMonths.end() ) 
	    std::cout <<  timepoint  << " intersects bad months" << std::endl;
    if ( !icl::is_empty(_badMonths & timepoint) ) 
	    std::cout <<  timepoint  << " intersects bad months" << std::endl;
    if ( !(_badMonths &=  timepoint).empty() ) 
	    std::cout <<  timepoint  << " intersects bad months" << std::endl;
} 

