/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_polymorphic2.hpp

// (C) Copyright 2009 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution
namespace boost {
namespace archive {
    class polymorphic_oarchive;
    class polymorphic_iarchive;
}
}

struct A {
public:
    A() {}
    virtual ~A() {}

    void serialize(
        boost::archive::polymorphic_oarchive &ar, 
        const unsigned int /*version*/
    );
    void serialize(
        boost::archive::polymorphic_iarchive &ar, 
        const unsigned int /*version*/
    );

    int i;
};

struct B : A {
    void serialize(
        boost::archive::polymorphic_oarchive &ar, 
        const unsigned int /*version*/
    );
    void serialize(
        boost::archive::polymorphic_iarchive &ar, 
        const unsigned int /*version*/
    );
};
