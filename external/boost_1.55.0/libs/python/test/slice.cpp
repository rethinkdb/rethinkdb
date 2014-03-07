#include <boost/python.hpp>
#include <boost/python/slice.hpp>
#include <boost/python/str.hpp>
#include <vector>

// Copyright (c) 2004 Jonathan Brandmeyer
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

using namespace boost::python;

#if BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x580)) || BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1500))
# define make_tuple boost::python::make_tuple
#endif 

// These checks are only valid under Python 2.3
// (rich slicing wasn't supported for builtins under Python 2.2)
bool check_string_rich_slice()
{
    object s("hello, world");

    // default slice
    if (s[slice()] != "hello, world")
        return false;

    // simple reverse
    if (s[slice(_,_,-1)] != "dlrow ,olleh")
        return false;

    // reverse with mixed-sign offsets
    if (s[slice(-6,1,-1)] != " ,oll")
        return false;

    // all of the object.cpp check_string_slice() checks should work
    // with the form that omits the step argument.
    if (s[slice(_,-3)] != "hello, wo")
        return false;
    if (s[slice(-3,_)] != "rld")
        return false;
    if (", " != s[slice(5,7)])
        return false;

    return s[slice(2,-1)][slice(1,-1)]  == "lo, wor";
}

// Tried to get more info into the error message (actual array
// contents) but Numeric complains that treating an array as a boolean
// value doesn't make any sense.
#define ASSERT_EQUAL( e1, e2 ) \
    if (!all((e1) == (e2)))                                                             \
        return "assertion failed: " #e1 " == " #e2 "\nLHS:\n%s\nRHS:\n%s" % make_tuple(e1,e2);    \
else

// These tests work with Python 2.2, but you must have Numeric installed.
object check_numeric_array_rich_slice(
    char const* module_name, char const* array_type_name, object all)
{
    using numeric::array;
    array::set_module_and_type(module_name, array_type_name);
    
    array original = array( make_tuple( make_tuple( 11, 12, 13, 14),
                                        make_tuple( 21, 22, 23, 24),
                                        make_tuple( 31, 32, 33, 34),
                                        make_tuple( 41, 42, 43, 44)));
    array upper_left_quadrant = array( make_tuple( make_tuple( 11, 12),
                                                   make_tuple( 21, 22)));
    array odd_cells = array( make_tuple( make_tuple( 11, 13),
                                          make_tuple( 31, 33)));
    array even_cells = array( make_tuple( make_tuple( 22, 24),
                                           make_tuple( 42, 44)));
    array lower_right_quadrant_reversed = array(
        make_tuple( make_tuple(44, 43),
                    make_tuple(34, 33)));

    // The following comments represent equivalent Python expressions used
    // to validate the array behavior.
    // original[::] == original
    ASSERT_EQUAL(original[slice()],original);
        
    // original[:2,:2] == array( [[11, 12], [21, 22]])
    ASSERT_EQUAL(original[make_tuple(slice(_,2), slice(_,2))],upper_left_quadrant);
        
    // original[::2,::2] == array( [[11, 13], [31, 33]])
    ASSERT_EQUAL(original[make_tuple( slice(_,_,2), slice(_,_,2))],odd_cells);
    
    // original[1::2, 1::2] == array( [[22, 24], [42, 44]])
    ASSERT_EQUAL(original[make_tuple( slice(1,_,2), slice(1,_,2))],even_cells);

    // original[:-3:-1, :-3,-1] == array( [[44, 43], [34, 33]])
    ASSERT_EQUAL(original[make_tuple( slice(_,-3,-1), slice(_,-3,-1))],lower_right_quadrant_reversed);

    return object(1);
}

// Verify functions accepting a slice argument can be called
bool accept_slice( slice) { return true; }

#if BOOST_WORKAROUND( BOOST_MSVC, BOOST_TESTED_AT(1400)) \
    || BOOST_WORKAROUND( BOOST_INTEL_WIN, == 710)
int check_slice_get_indices(slice index);
#endif
int check_slice_get_indices(
#if !BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))
    const
#endif 
    slice index)
{
    // A vector of integers from [-5, 5].
    std::vector<int> coll(11);
    typedef std::vector<int>::iterator coll_iterator;
    
    for (coll_iterator i = coll.begin(); i != coll.end(); ++i) {
        *i = i - coll.begin() - 5;
    }
    
    slice::range<std::vector<int>::iterator> bounds;
    try {
        bounds = index.get_indices(coll.begin(), coll.end());
    }
    catch (std::invalid_argument) {
        return 0;
    }
    int sum = 0;
    while (bounds.start != bounds.stop) {
        sum += *bounds.start;
        std::advance( bounds.start, bounds.step);
    }
    sum += *bounds.start;
    return sum;
}


BOOST_PYTHON_MODULE(slice_ext)
{
    def( "accept_slice", accept_slice);
    def( "check_numeric_array_rich_slice", check_numeric_array_rich_slice);
    def( "check_string_rich_slice", check_string_rich_slice);
    def( "check_slice_get_indices", check_slice_get_indices);
}
