// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#include "test_utils.hpp"
#include <boost/any.hpp>
#include <boost/range.hpp>
#include <list>
#include <cmath>
#include <iostream>

// If using VC, disable some warnings that trip in boost::serialization bowels
#ifdef BOOST_MSVC
    #pragma warning(disable:4267)   // Narrowing conversion
    #pragma warning(disable:4996)   // Deprecated functions
#endif

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/property_tree/ptree_serialization.hpp>

// Predicate for sorting keys
template<class Ptree>
struct SortPred
{
    bool operator()(const typename Ptree::value_type &v1,
                    const typename Ptree::value_type &v2) const
    {
        return v1.first < v2.first;
    }
};

// Predicate for sorting keys in reverse
template<class Ptree>
struct SortPredRev
{
    bool operator()(const typename Ptree::value_type &v1,
                    const typename Ptree::value_type &v2) const
    {
        return v1.first > v2.first;
    }
};

// Custom translator that works with boost::any instead of std::string
template <typename E>
struct any_translator
{
    typedef boost::any internal_type;
    typedef E external_type;

    boost::optional<E> get_value(const internal_type &v) {
        if(const E *p = boost::any_cast<E>(&v)) {
            return *p;
        }
        return boost::optional<E>();
    }
    boost::optional<internal_type> put_value(const E &v) {
        return boost::any(v);
    }
};

namespace boost { namespace property_tree {
    template <typename E>
    struct translator_between<boost::any, E>
    {
        typedef any_translator<E> type;
    };
}}

// Include char tests, case sensitive
#define CHTYPE char
#define T(s) s
#define PTREE boost::property_tree::ptree
#define NOCASE 0
#define WIDECHAR 0
#   include "test_property_tree.hpp"
#undef CHTYPE
#undef T
#undef PTREE
#undef NOCASE
#undef WIDECHAR

// Include wchar_t tests, case sensitive
#ifndef BOOST_NO_CWCHAR
#   define CHTYPE wchar_t
#   define T(s) L ## s
#   define PTREE boost::property_tree::wptree
#   define NOCASE 0
#   define WIDECHAR 1
#       include "test_property_tree.hpp"
#   undef CHTYPE
#   undef T
#   undef PTREE
#   undef NOCASE
#   undef WIDECHAR
#endif

// Include char tests, case insensitive
#define CHTYPE char
#define T(s) s
#define PTREE boost::property_tree::iptree
#define NOCASE 1
#   define WIDECHAR 0
#   include "test_property_tree.hpp"
#undef CHTYPE
#undef T
#undef PTREE
#undef NOCASE
#undef WIDECHAR

// Include wchar_t tests, case insensitive
#ifndef BOOST_NO_CWCHAR
#   define CHTYPE wchar_t
#   define T(s) L ## s
#   define PTREE boost::property_tree::wiptree
#   define NOCASE 1
#   define WIDECHAR 1
#       include "test_property_tree.hpp"
#   undef CHTYPE
#   undef T
#   undef PTREE
#   undef NOCASE
#   undef WIDECHAR
#endif

int test_main(int, char *[])
{
    
    using namespace boost::property_tree;
    
    // char tests, case sensitive
    {
        ptree *pt = 0;
        test_debug(pt);
        test_constructor_destructor_assignment(pt);
        test_insertion(pt);
        test_erasing(pt);
        test_clear(pt);
        test_pushpop(pt);
        test_container_iteration(pt);
        test_swap(pt);
        test_sort_reverse(pt);
        test_case(pt);
        test_comparison(pt);
        test_front_back(pt);
        test_get_put(pt);
        test_get_child_put_child(pt);
        test_equal_range(pt);
        test_path_separator(pt);
        test_path(pt);
        test_precision(pt);
        test_locale(pt);
        test_custom_data_type(pt);
        test_empty_size_max_size(pt);
        test_ptree_bad_path(pt);
        test_ptree_bad_data(pt);
        test_serialization(pt);
        test_bool(pt);
        test_char(pt);
        test_sort(pt);
        test_leaks(pt);                  // must be a final test
    }
#if 0
    // wchar_t tests, case sensitive
#ifndef BOOST_NO_CWCHAR
    {
        wptree *pt = 0;
        test_debug(pt);
        test_constructor_destructor_assignment(pt);
        test_insertion(pt);
        test_erasing(pt);
        test_clear(pt);
        test_pushpop(pt);
        test_container_iteration(pt);
        test_swap(pt);
        test_sort_reverse(pt);
        test_case(pt);
        test_comparison(pt);
        test_front_back(pt);
        test_get_put(pt);
        test_get_child_put_child(pt);
        test_equal_range(pt);
        test_path_separator(pt);
        test_path(pt);
        test_precision(pt);
        test_locale(pt);
        test_custom_data_type(pt);
        test_empty_size_max_size(pt);
        test_ptree_bad_path(pt);
        test_ptree_bad_data(pt);
        test_serialization(pt);
        test_bool(pt);
        test_char(pt);
        test_sort(pt);
        test_leaks(pt);                  // must be a final test
    }
#endif

    // char tests, case insensitive
    {
        iptree *pt = 0;
        test_debug(pt);
        test_constructor_destructor_assignment(pt);
        test_insertion(pt);
        test_erasing(pt);
        test_clear(pt);
        test_pushpop(pt);
        test_container_iteration(pt);
        test_swap(pt);
        test_sort_reverse(pt);
        test_case(pt);
        test_comparison(pt);
        test_front_back(pt);
        test_get_put(pt);
        test_get_child_put_child(pt);
        test_equal_range(pt);
        test_path_separator(pt);
        test_path(pt);
        test_precision(pt);
        test_locale(pt);
        test_custom_data_type(pt);
        test_empty_size_max_size(pt);
        test_ptree_bad_path(pt);
        test_ptree_bad_data(pt);
        test_serialization(pt);
        test_bool(pt);
        test_char(pt);
        test_sort(pt);
        test_leaks(pt);                  // must be a final test
    }

    // wchar_t tests, case insensitive
#ifndef BOOST_NO_CWCHAR
    {
        wiptree *pt = 0;
        test_debug(pt);
        test_constructor_destructor_assignment(pt);
        test_insertion(pt);
        test_erasing(pt);
        test_clear(pt);
        test_pushpop(pt);
        test_container_iteration(pt);
        test_swap(pt);
        test_sort_reverse(pt);
        test_case(pt);
        test_comparison(pt);
        test_front_back(pt);
        test_get_put(pt);
        test_get_child_put_child(pt);
        test_equal_range(pt);
        test_path_separator(pt);
        test_path(pt);
        test_precision(pt);
        test_locale(pt);
        test_custom_data_type(pt);
        test_empty_size_max_size(pt);
        test_ptree_bad_path(pt);
        test_ptree_bad_data(pt);
        test_serialization(pt);
        test_bool(pt);
        test_char(pt);
        test_sort(pt);
        test_leaks(pt);                  // must be a final test
    }
#endif
#endif
    return 0;
}
