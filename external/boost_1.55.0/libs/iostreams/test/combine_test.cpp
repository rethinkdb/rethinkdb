/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.
 *
 * Verifies that the close() member functions of filters and devices
 * are called with the correct arguments in the correct order when 
 * they are combined using combine().
 *
 * File:        libs/iostreams/test/combine_test.cpp
 * Date:        Sun Jan 06 01:37:37 MST 2008
 * Copyright:   2007-2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 */

#include <boost/iostreams/chain.hpp>
#include <boost/iostreams/combine.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>  
#include "detail/closable.hpp"
#include "detail/operation_sequence.hpp"

using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;
namespace io = boost::iostreams;

void combine_test()
{
    // Combine a source and a sink
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        ch.push(
            io::combine(
                closable_device<input>(seq.new_operation(1)),
                closable_device<output>(seq.new_operation(2))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Combine two bidirectional devices
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        ch.push(
            io::combine(
                closable_device<bidirectional>(
                    seq.new_operation(1),
                    seq.new_operation(2)
                ),
                closable_device<bidirectional>(
                    seq.new_operation(3),
                    seq.new_operation(4)
                )
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Combine two seekable devices
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        ch.push(
            io::combine(
                closable_device<seekable>(seq.new_operation(1)),
                closable_device<seekable>(seq.new_operation(2))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Combine an input filter and an output filter
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        ch.push(
            io::combine(
                closable_filter<input>(seq.new_operation(2)),
                closable_filter<output>(seq.new_operation(3))
            )
        );
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Combine two bidirectional filters
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        ch.push(
            io::combine(
                closable_filter<bidirectional>(
                    seq.new_operation(2),
                    seq.new_operation(3)
                ),
                closable_filter<bidirectional>(
                    seq.new_operation(4),
                    seq.new_operation(5)
                )
            )
        );
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(6)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Combine two seekable filters
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        ch.push(
            io::combine(
                closable_filter<seekable>(seq.new_operation(2)),
                closable_filter<seekable>(seq.new_operation(3))
            )
        );
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Combine a dual-use filter and an input filter
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        operation             dummy;
        ch.push(
            io::combine(
                closable_filter<input>(seq.new_operation(2)),
                closable_filter<dual_use>(
                    dummy,
                    seq.new_operation(3)
                )
            )
        );
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Combine a dual-use filter and an output filter
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        operation             dummy;
        ch.push(
            io::combine(
                closable_filter<dual_use>(
                    seq.new_operation(2),
                    dummy
                ),
                closable_filter<output>(seq.new_operation(3))
            )
        );
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Combine two dual-use filters
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        operation             dummy;
        ch.push(
            io::combine(
                closable_filter<dual_use>(
                    seq.new_operation(2),
                    dummy
                ),
                closable_filter<dual_use>(
                    dummy,
                    seq.new_operation(3)
                )
            )
        );
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("combine test");
    test->add(BOOST_TEST_CASE(&combine_test));
    return test;
}
