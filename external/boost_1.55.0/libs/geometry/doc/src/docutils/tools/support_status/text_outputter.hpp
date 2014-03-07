// Boost.Geometry (aka GGL, Generic Geometry Library)
// Tool reporting Implementation Support Status in QBK or plain text format

// Copyright (c) 2011-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SUPPORT_STATUS_TEXT_OUTPUTTER_HPP
#define BOOST_GEOMETRY_SUPPORT_STATUS_TEXT_OUTPUTTER_HPP


struct text_outputter
{
    explicit text_outputter(const std::string&) {}

    static inline void ok() { std::cout << "OK\t"; }
    static inline void nyi() { std::cout << "-\t"; }
    static inline void header(std::string const& algo) { std::cout << algo << std::endl; }

    template <typename T>
    static inline void table_header() {  }
    static inline void table_header() {  }

    static inline void table_footer() { std::cout << std::endl; }

    template <typename G>
    static inline void begin_row() {}

    static inline void end_row() { std::cout << std::endl; }

};

#endif // BOOST_GEOMETRY_SUPPORT_STATUS_TEXT_OUTPUTTER_HPP
