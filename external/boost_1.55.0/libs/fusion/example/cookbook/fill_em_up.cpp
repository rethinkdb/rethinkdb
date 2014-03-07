/*=============================================================================
    Copyright (c) 2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    Problem:

    So... you have an input sequence I and a target vector R. You want to
    copy I into R. But, I may have less elements than the result vector R.
    For those elements not in R, you want them to be default constructed.

    Here's a case:
    
        I: list<double, std::string>
        R: vector<double, std::string, int, short>
    
    You want the elements at the right of I not in R (i.e. int, short)
    default constructed. Those at the left, found in both I and R, you want
    to simply copy from I.
    
    Of course you want to be able to handle any type of I and R.

==============================================================================*/

// We'll use these containers as examples
#include <boost/fusion/container/list.hpp>
#include <boost/fusion/container/vector.hpp>

// For doing I/O
#include <boost/fusion/sequence/io.hpp>

// We'll use join and advance for processing
#include <boost/fusion/algorithm/transformation/join.hpp>
#include <boost/fusion/iterator/advance.hpp>

// The fusion <--> MPL link header
#include <boost/fusion/mpl.hpp>

// Same-o same-o
#include <iostream>
#include <string>

int
main()
{
    using namespace boost::fusion;
    using namespace boost;
    
    // Let's specify our own tuple delimeters for nicer printing
    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    // Here's your input sequence
    typedef list<double, std::string> I;
    I i(123.456, "Hello");
    
    // Here's your output sequence. For now, it is just a typedef
    typedef vector<double, std::string, int, short> R;

    // Let's get the sizes of the sequences. Yeah, you already know that.
    // But with templates, you are simply given, say, R and I, corresponding 
    // to the types of the sequences. You'll have to deal with it generically.
    static int const r_size = result_of::size<R>::value;
    static int const i_size = result_of::size<I>::value;

    // Make sure that I has no more elements than R
    // Be nice and catch obvious errors earlier rather than later.
    // Without this assert, the mistake will still be caught by Fusion,
    // but the error will point to somewhere really obscure.
    BOOST_STATIC_ASSERT(i_size <= r_size);

    // Let's get the begin and end iterator types of the output sequence
    // There's no actual vector yet. We just want to know the types.
    typedef result_of::begin<R>::type r_begin;
    typedef result_of::end<R>::type r_end;

    // Let's skip i_size elements from r_begin. Again, we just want to know the type.
    typedef result_of::advance_c<r_begin, i_size>::type r_advance;

    // Now, make MPL iterators from r_advance and r_end. Ditto, just types.
    typedef mpl::fusion_iterator<r_advance> mpl_r_advance;
    typedef mpl::fusion_iterator<r_end> mpl_r_end;

    // Make an mpl::iterator_range from the MPL iterators we just created 
    // You guessed it! --just a type.
    typedef mpl::iterator_range<mpl_r_advance, mpl_r_end> tail;

    // Use join to join the input sequence and our mpl::iterator_range
    // Our mpl::iterator_range is 'tail'. Here, we'll actually instantiate
    // 'tail'. Notice that this is a flyweight object, typically just 1 byte
    // in size -- it doesn't really hold any data, but is a fully conforming
    // sequence nonetheless. When asked to return its elements, 'tail' returns
    // each element default constructed. Breeds like a rabbit!

    // Construct R from the joined sequences:
    R r(join(i, tail()));
    
    // Then finally, print the result:
    std::cout << r << std::endl;

    return 0;
}

