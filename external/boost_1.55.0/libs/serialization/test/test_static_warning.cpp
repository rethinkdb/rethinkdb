//  (C) Copyright Jonathan Turkanis 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

// note: this is a compile only test.

#include <boost/config.hpp> // BOOST_STATIC_CONST
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_polymorphic.hpp>

#include <boost/serialization/static_warning.hpp>

typedef char a1[2];
typedef char a2[3];

class polymorphic {
    virtual ~polymorphic();
};

class non_polymorphic {
};

template<class T>
int f(){
    BOOST_STATIC_WARNING(T::value);
    return 0;
}

struct A {
    BOOST_STATIC_CONSTANT(bool, value = false);
};

/////////////////////////////////////////////////////////////////////////
// compilation of this program should show a total of 10 warning messages

// should show NO warning message
BOOST_STATIC_WARNING(true);

// the following should show 5 warning message
int x = f<A>();  // Warn
int y = f<boost::is_polymorphic<non_polymorphic> >(); // Warn.
int z = f<boost::is_polymorphic<polymorphic> >();

BOOST_STATIC_WARNING(sizeof(a1) == sizeof(a2)); // Warn.
BOOST_STATIC_WARNING(sizeof(a1) != sizeof(a1)); // Warn.
BOOST_STATIC_WARNING(! boost::is_polymorphic<polymorphic>::value); // Warn.
BOOST_STATIC_WARNING(boost::is_polymorphic<non_polymorphic>::value); // Warn.

int main(int /* argc */, char * /* argv */[]){
    // should show NO warning message
    BOOST_STATIC_WARNING(true);

    // the following should show 5 warning message
    f<A>();
    BOOST_STATIC_WARNING(sizeof(a1) == sizeof(a2)); // Warn.
    BOOST_STATIC_WARNING(sizeof(a1) != sizeof(a1)); // Warn.
    BOOST_STATIC_WARNING(! boost::is_polymorphic<polymorphic>::value); // Warn.
    BOOST_STATIC_WARNING(boost::is_polymorphic<non_polymorphic>::value); // Warn.
    return 0;
}
