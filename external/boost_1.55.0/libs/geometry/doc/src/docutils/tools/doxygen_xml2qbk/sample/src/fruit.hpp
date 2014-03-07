// Boost.Geometry (aka GGL, Generic Geometry Library)
// doxygen_xml2qbk Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef FRUIT_HPP
#define FRUIT_HPP


#include <iostream>
#include <string>

/*!
\defgroup fruit fruit: Fruit group
*/

namespace fruit
{

/*!
\brief Enumeration to select color
\ingroup fruit
*/
enum fruit_color
{
    /// A yellow or yellowish color
    yellow = 1,
    /// A green or greeny color
    green = 2,
    /// An orange color
    orange = 3
};


/*!
\brief Any metafunction (with type)
\ingroup fruit
*/
struct fruit_type
{
    /// the type
    typedef int type;
};

/*!
\brief Any metafunction (with value)
\ingroup fruit
*/
struct fruit_value
{
    /// the value
    static const fruit_color value = yellow;
};



/// Rose (Rosaceae)
class rose {};

/*!
\brief An apple
\details The apple is the pomaceous fruit of the apple tree,
    species Malus domestica in the rose family (Rosaceae)
\tparam String the string-type (string,wstring,utf8-string,etc)

\qbk{before.synopsis,
[heading Model of]
Fruit Concept
}
*/
template <typename String = std::string>
class apple : public rose
{
    String sort;

public :
    /// constructor
    apple(String const& s) : sort(s) {}

    /// Get the name
    // (more doxygen e.g. @return, etc)
    String const& name() const { return sort; }
};


/*!
\brief Eat it
\ingroup fruit
\details Eat the fruit
\param fruit the fruit
\tparam T the fruit type

\qbk{
[heading Example]
[apple]
[apple_output]
}
*/
template <typename T>
void eat(T const& fruit)
{
    std::cout << fruit.name() << std::endl;
}

}


#endif
