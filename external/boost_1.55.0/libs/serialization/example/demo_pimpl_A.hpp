#ifndef BOOST_SERIALIZATION_EXAMPLE_DEMO_PIMPL_A_HPP
#define BOOST_SERIALIZATION_EXAMPLE_DEMO_PIMPL_A_HPP

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// demo_pimpl_A.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// class whose declaration is hidden by a pointer
struct B;

struct A {
    // class a contains a pointer to a "hidden" declaration
    B *pimpl;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version);
    A();
    ~A();
};

#endif // BOOST_SERIALIZATION_EXAMPLE_DEMO_PIMPL_A_HPP
