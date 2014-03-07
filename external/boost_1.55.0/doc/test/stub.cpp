/*=============================================================================
    Copyright (c) 2006 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <string>

//[ bar
//` This is the [*/bar/] function
std::string bar()
{
    // return 'em, bar man!
    return "bar";
}
//]

//[ foo
/*` This is the [*['foo]] function. */
std::string foo()
{
    // return 'em, foo man!
    return "foo";
}
//]

//[ foo_bar
std::string foo_bar() /*< The /Mythical/ FooBar.
                      See [@http://en.wikipedia.org/wiki/Foobar Foobar for details] >*/
{
    return "foo-bar"; /*< return 'em, foo-bar man! >*/
}
//]

//[ class_
class x
{
public:

    /*<< Constructor >>*/
    x() : n(0)
    {
    }

    /*<< Destructor >>*/
    ~x()
    {
    }

    /*<< Get the `n` member variable >>*/
    int get() const
    {
        return n; /*<- this will be ignored by quickbook ->*/
    }

    /*<< Set the `n` member variable >>*/
    void set(int n_)
    {
        n = n_;
    }
//<- this will be ignored by quickbook
private:

    int n;
//->
};
//]
