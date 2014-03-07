/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.
 *
 * Verifies that the close() member functions of filters and devices
 * are called with the correct arguments in the correct order when 
 * used with chains and streams.
 *
 * File:        libs/iostreams/test/close_test.cpp
 * Date:        Sun Dec 09 16:12:23 MST 2007
 * Copyright:   2007 CodeRage
 * Author:      Jonathan Turkanis
 */

#include <boost/iostreams/chain.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>  
#include "detail/closable.hpp"
#include "detail/operation_sequence.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;
namespace io = boost::iostreams;

void input_chain_test()
{
    // Test input filter and device
    {
        operation_sequence          seq;
        filtering_streambuf<input>  ch;

        // Test chain::pop()
        ch.push(closable_filter<input>(seq.new_operation(2)));
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test bidirectional filter and device
    {
        operation_sequence          seq;
        filtering_streambuf<input>  ch;

        // Test chain::pop()
        ch.push(
            closable_filter<bidirectional>(
                seq.new_operation(2),
                seq.new_operation(3)
            )
        );
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test seekable filter and device
    {
        operation_sequence          seq;
        filtering_streambuf<input>  ch;

        // Test chain::pop()
        ch.push(closable_filter<seekable>(seq.new_operation(1)));
        ch.push(closable_device<seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test dual-user filter
    {
        operation_sequence          seq;
        filtering_streambuf<input>  ch;
        operation                   dummy;

        // Test chain::pop()
        ch.push(
            closable_filter<dual_use>(
                seq.new_operation(2), 
                dummy
            )
        );
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
        
        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test direct source
    {
        operation_sequence          seq;
        filtering_streambuf<input>  ch;

        // Test chain::pop()
        ch.push(closable_filter<input>(seq.new_operation(2)));
        ch.push(closable_device<direct_input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
        
        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<direct_input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<direct_input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test direct bidirectional device
    {
        operation_sequence          seq;
        filtering_streambuf<input>  ch;

        // Test chain::pop()
        ch.push(closable_filter<input>(seq.new_operation(2)));
        ch.push(
            closable_device<direct_bidirectional>(
                seq.new_operation(1),
                seq.new_operation(3)
            )
        );
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(
            closable_device<direct_bidirectional>(
                seq.new_operation(1),
                seq.new_operation(3)
            )
        );
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(
            closable_device<direct_bidirectional>(
                seq.new_operation(1),
                seq.new_operation(3)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test direct seekable device
    {
        operation_sequence          seq;
        filtering_streambuf<input>  ch;

        // Test chain::pop()
        ch.push(closable_filter<input>(seq.new_operation(1)));
        ch.push(closable_device<direct_seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<direct_seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<direct_seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}

void output_chain_test()
{
    // Test output filter and device
    {
        operation_sequence           seq;
        filtering_streambuf<output>  ch;

        // Test chain::pop()
        ch.push(closable_filter<output>(seq.new_operation(1)));
        ch.push(closable_device<output>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<output>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<output>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test bidirectional filter and device
    {
        operation_sequence           seq;
        filtering_streambuf<output>  ch;
        
        // Test chain::pop()
        ch.push(
            closable_filter<bidirectional>(
                seq.new_operation(2),
                seq.new_operation(3)
            )
        );
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test seekable filter and device
    {
        operation_sequence           seq;
        filtering_streambuf<output>  ch;

        // Test chain::pop()
        ch.push(closable_filter<seekable>(seq.new_operation(1)));
        ch.push(closable_device<seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test dual-user filter
    {
        operation_sequence           seq;
        filtering_streambuf<output>  ch;
        operation                    dummy;

        // Test chain::pop()
        ch.push(
            closable_filter<dual_use>(
                dummy, 
                seq.new_operation(1)
            )
        );
        ch.push(closable_device<output>(seq.new_operation(3)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<output>(seq.new_operation(3)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<output>(seq.new_operation(3)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test direct sink
    {
        operation_sequence           seq;
        filtering_streambuf<output>  ch;

        // Test chain::pop()
        ch.push(closable_filter<output>(seq.new_operation(1)));
        ch.push(closable_device<direct_output>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<direct_output>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<direct_output>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test direct bidirectional device
    {
        operation_sequence           seq;
        filtering_streambuf<output>  ch;

        // Test chain::pop()
        ch.push(closable_filter<output>(seq.new_operation(2)));
        ch.push(
            closable_device<direct_bidirectional>(
                seq.new_operation(1),
                seq.new_operation(3)
            )
        );
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(
            closable_device<direct_bidirectional>(
                seq.new_operation(1),
                seq.new_operation(3)
            )
        );
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(
            closable_device<direct_bidirectional>(
                seq.new_operation(1),
                seq.new_operation(3)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test direct seekable device
    {
        operation_sequence           seq;
        filtering_streambuf<output>  ch;

        // Test chain::pop()
        ch.push(closable_filter<output>(seq.new_operation(1)));
        ch.push(closable_device<direct_seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<direct_seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<direct_seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}

void bidirectional_chain_test()
{
    // Test bidirectional filter and device
    {
        operation_sequence                  seq;
        filtering_streambuf<bidirectional>  ch;
        
        // Test chain::pop()
        ch.push(
            closable_filter<bidirectional>(
                seq.new_operation(2),
                seq.new_operation(3)
            )
        );
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test direct bidirectional device
    {
        operation_sequence                  seq;
        filtering_streambuf<bidirectional>  ch;

        // Test chain::pop()
        ch.push(
            closable_filter<bidirectional>(
                seq.new_operation(2),
                seq.new_operation(3)
            )
        );
        ch.push(
            closable_device<direct_bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(
            closable_device<direct_bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(
            closable_device<direct_bidirectional>(
                seq.new_operation(1),
                seq.new_operation(4)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}

void seekable_chain_test()
{
    // Test seekable filter and device
    {
        operation_sequence             seq;
        filtering_streambuf<seekable>  ch;

        // Test chain::pop()
        ch.push(closable_filter<seekable>(seq.new_operation(1)));
        ch.push(closable_device<seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test direct seekable device
    {
        operation_sequence             seq;
        filtering_streambuf<seekable>  ch;

        // Test chain::pop()
        ch.push(closable_filter<seekable>(seq.new_operation(1)));
        ch.push(closable_device<direct_seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.pop());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and io::close()
        seq.reset();
        ch.push(closable_device<direct_seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(io::close(ch));
        BOOST_CHECK_OPERATION_SEQUENCE(seq);

        // Test filter reuse and chain::reset()
        seq.reset();
        ch.push(closable_device<direct_seekable>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}

void stream_test()
{
    // Test source
    {
        operation_sequence                seq;
        stream< closable_device<input> >  str;
        str.open(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(str.close());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test sink
    {
        operation_sequence                 seq;
        stream< closable_device<output> >  str;
        str.open(closable_device<output>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(str.close());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test bidirectional device
    {
        operation_sequence                        seq;
        stream< closable_device<bidirectional> >  str;
        str.open(
            closable_device<bidirectional>(
                seq.new_operation(1),
                seq.new_operation(2)
            )
        );
        BOOST_CHECK_NO_THROW(str.close());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test seekable device
    {
        operation_sequence                   seq;
        stream< closable_device<seekable> >  str;
        str.open(closable_device<seekable>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(str.close());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("execute test");
    test->add(BOOST_TEST_CASE(&input_chain_test));
    test->add(BOOST_TEST_CASE(&output_chain_test));
    test->add(BOOST_TEST_CASE(&bidirectional_chain_test));
    test->add(BOOST_TEST_CASE(&seekable_chain_test));
    test->add(BOOST_TEST_CASE(&stream_test));
    return test;
}
