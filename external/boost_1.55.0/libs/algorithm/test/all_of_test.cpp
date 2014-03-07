/* 
   Copyright (c) Marshall Clow 2010-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <boost/config.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <functional>
#include <vector>
#include <list>

template<typename T>
struct is_ : public std::unary_function<T, bool> {
    is_ ( T v ) : val_ ( v ) {}
    ~is_ () {}
    bool operator () ( T comp ) const { return val_ == comp; }
private:
    is_ (); // need a value

    T val_;
    };

namespace ba = boost::algorithm;

void test_all ()
{
//  Note: The literal values here are tested against directly, careful if you change them:
    int some_numbers[] = { 1, 1, 1, 18, 10 };
    std::vector<int> vi(some_numbers, some_numbers + 5);
    std::list<int>   li(vi.begin(), vi.end ());
    
    int some_letters[] = { 'a', 'q', 'n', 'y', 'n' };
    std::vector<char> vc(some_letters, some_letters + 5);
    
    
    BOOST_CHECK (!ba::all_of_equal ( vi,                                  1 ));
    BOOST_CHECK (!ba::all_of       ( vi,                       is_<int> ( 1 )));
    BOOST_CHECK (!ba::all_of_equal ( vi.begin(),     vi.end(),            1 ));
    BOOST_CHECK (!ba::all_of       ( vi.begin(),     vi.end(), is_<int> ( 1 )));
    
    BOOST_CHECK (!ba::all_of_equal ( vi,                                  0 ));
    BOOST_CHECK (!ba::all_of       ( vi,                       is_<int> ( 0 )));
    BOOST_CHECK (!ba::all_of_equal ( vi.begin(),     vi.end(),            0 ));
    BOOST_CHECK (!ba::all_of       ( vi.begin(),     vi.end(), is_<int> ( 0 )));

    BOOST_CHECK ( ba::all_of_equal ( vi.end(),       vi.end(),            0 ));
    BOOST_CHECK ( ba::all_of       ( vi.end(),       vi.end(), is_<int> ( 0 )));

    BOOST_CHECK ( ba::all_of_equal ( vi.begin(), vi.begin () + 3,            1 ));
    BOOST_CHECK ( ba::all_of       ( vi.begin(), vi.begin () + 3, is_<int> ( 1 )));
    
    BOOST_CHECK ( ba::all_of_equal ( vc.begin() + 1, vc.begin() + 2,             'q' ));
    BOOST_CHECK ( ba::all_of       ( vc.begin() + 1, vc.begin() + 2, is_<char> ( 'q' )));

    BOOST_CHECK (!ba::all_of_equal ( vc,             '!' ));
    BOOST_CHECK (!ba::all_of       ( vc, is_<char> ( '!' )));

    BOOST_CHECK ( ba::all_of_equal ( vi.begin(), vi.begin(),   1 ));
    BOOST_CHECK ( ba::all_of_equal ( vc.begin(), vc.begin(), 'a' ));
    BOOST_CHECK ( ba::all_of       ( vi.begin(), vi.begin(), is_<int>  (   1 )));
    BOOST_CHECK ( ba::all_of       ( vc.begin(), vc.begin(), is_<char> ( 'a' )));

    BOOST_CHECK (!ba::all_of_equal ( li,                                  1 ));
    BOOST_CHECK (!ba::all_of       ( li,                       is_<int> ( 1 )));
    BOOST_CHECK (!ba::all_of_equal ( li.begin(),     li.end(),            1 ));
    BOOST_CHECK (!ba::all_of       ( li.begin(),     li.end(), is_<int> ( 1 )));
    
    std::list<int>::iterator l_iter = li.begin ();
    l_iter++; l_iter++; l_iter++;
    BOOST_CHECK ( ba::all_of_equal ( li.begin(), l_iter,            1 ));
    BOOST_CHECK ( ba::all_of       ( li.begin(), l_iter, is_<int> ( 1 )));
    
}


BOOST_AUTO_TEST_CASE( test_main )
{
  test_all ();
}
