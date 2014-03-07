/*=============================================================================
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_SPIRIT_QUICKBOOK_VALUES_TAGS_HPP)
#define BOOST_SPIRIT_QUICKBOOK_VALUES_TAGS_HPP

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/range/irange.hpp>
#include <cassert>

#define QUICKBOOK_VALUE_TAGS(tags_name, start_index, values) \
    struct tags_name { \
        enum tags_name##_enum { \
            previous_index = start_index - 1, \
            BOOST_PP_SEQ_ENUM(values), \
            end_index \
        }; \
        \
        static char const* name(int value) { \
            switch(value) {\
            case 0: \
                return "null"; \
            BOOST_PP_SEQ_FOR_EACH(QUICKBOOK_VALUE_CASE, _, values) \
            default: \
                assert(false); return ""; \
            }; \
        } \
        \
        typedef boost::integer_range<int> range_type; \
        static range_type tags() { return boost::irange(start_index, (int) end_index); } \
    };

#define QUICKBOOK_VALUE_CASE(r, _, value) \
    case value: return BOOST_PP_STRINGIZE(value);

#define QUICKBOOK_VALUE_NAMED_TAGS(tags_name, start_index, values) \
    struct tags_name { \
        enum tags_name##_enum { \
            previous_index = start_index - 1 \
            BOOST_PP_SEQ_FOR_EACH(QUICKBOOK_VALUE_NAMED_ENUM, _, values), \
            end_index \
        }; \
        \
        static char const* name(int value) { \
            switch(value) {\
            case 0: \
                return "null"; \
            BOOST_PP_SEQ_FOR_EACH(QUICKBOOK_VALUE_NAMED_CASE, _, values) \
            default: \
                assert(false); return ""; \
            }; \
        } \
        \
        typedef boost::integer_range<int> range_type; \
        static range_type tags() { return boost::irange(start_index, (int) end_index); } \
    };

#define QUICKBOOK_VALUE_NAMED_ENUM(r, _, value) \
    , BOOST_PP_SEQ_ELEM(0, value)

#define QUICKBOOK_VALUE_NAMED_CASE(r, _, value) \
    case BOOST_PP_SEQ_ELEM(0, value): return BOOST_PP_SEQ_ELEM(1, value);


#endif
