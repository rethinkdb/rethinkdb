// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: multifunction.cpp 83393 2013-03-10 03:48:33Z steven_watanabe $

//[multifunction
/*`
    (For the source of this example see
    [@boost:/libs/type_erasure/example/multifunction.cpp multifunction.cpp])

    This example implements an extension of Boost.Function that supports
    multiple signatures.

    [note This example uses C++11 features.  You'll need a
    recent compiler for it to work.]
 */

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/callable.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/variant.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/range/algorithm.hpp>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;
namespace phoenix = boost::phoenix;

// First of all we'll declare the multifunction template.
// multifunction is like Boost.Function but instead of
// taking one signature, it takes any number of them.
template<class... Sig>
using multifunction =
    any<
        mpl::vector<
            copy_constructible<>,
            typeid_<>,
            relaxed,
            callable<Sig>...
        >
    >;

// Let's use multifunction to process a variant.  We'll start
// by defining a simple recursive variant to use.
typedef boost::make_recursive_variant<
    int,
    double,
    std::string,
    std::vector<boost::recursive_variant_> >::type variant_type;
typedef std::vector<variant_type> vector_type;

// Now we'll define a multifunction that can operate
// on the leaf nodes of the variant.
typedef multifunction<void(int), void(double), void(std::string)> function_type;

class variant_handler
{
public:
    void handle(const variant_type& arg)
    {
        boost::apply_visitor(impl, arg);
    }
    void set_handler(function_type f)
    {
        impl.f = f;
    }
private:
    // A class that works with boost::apply_visitor
    struct dispatcher : boost::static_visitor<void>
    {
        // used for the leaves
        template<class T>
        void operator()(const T& t) { f(t); }
        // For a vector, we recursively operate on the elements
        void operator()(const vector_type& v)
        {
            boost::for_each(v, boost::apply_visitor(*this));
        }
        function_type f;
    };
    dispatcher impl;
};

int main() {
    variant_handler x;
    x.set_handler(std::cout << phoenix::val("Value: ") << phoenix::placeholders::_1 << std::endl);

    x.handle(1);
    x.handle(2.718);
    x.handle("The quick brown fox jumps over the lazy dog.");
    x.handle(vector_type{ 1.618, "Gallia est omnis divisa in partes tres", 42 });
}

//]
