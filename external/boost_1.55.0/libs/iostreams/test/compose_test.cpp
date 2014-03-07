// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <cctype>
#include <boost/iostreams/compose.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/closable.hpp"
#include "detail/operation_sequence.hpp"
#include "detail/filters.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp> // BCC 5.x.

using namespace std;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;
namespace io = boost::iostreams;

void read_composite()
{
    test_file          src1, src2;
    filtering_istream  first, second;

    // Test composite device
    first.push(toupper_filter());
    first.push(padding_filter('a'));
    first.push(file_source(src1.name(), in_mode));
    second.push( compose( toupper_filter(),
                          compose( padding_filter('a'),
                                   file_source(src1.name(), in_mode) ) ) );
    BOOST_CHECK_MESSAGE(
        compare_streams_in_chunks(first, second),
        "failed reading from a stdio_filter"
    );

    // Test composite filter
    first.reset();
    second.reset();
    first.push(toupper_filter());
    first.push(padding_filter('a'));
    first.push(file_source(src1.name(), in_mode));
    second.push( compose( compose( toupper_filter(),
                                   padding_filter('a') ),
                          file_source(src1.name(), in_mode) ) );
    BOOST_CHECK_MESSAGE(
        compare_streams_in_chunks(first, second),
        "failed reading from a stdio_filter"
    );
}

void write_composite()
{
    temp_file          dest1, dest2;
    filtering_ostream  out1, out2;

    // Test composite device
    out1.push(tolower_filter());
    out1.push(padding_filter('a'));
    out1.push(file_sink(dest1.name(), in_mode));
    out2.push( compose( tolower_filter(),
                        compose( padding_filter('a'),
                                 file_sink(dest2.name(), in_mode) ) ) );
    write_data_in_chunks(out1);
    write_data_in_chunks(out2);
    out1.reset();
    out2.reset();

    {
        ifstream first(dest1.name().c_str());
        ifstream second(dest2.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed writing to a stdio_filter"
        );
    }

    // Test composite filter
    out1.push(tolower_filter());
    out1.push(padding_filter('a'));
    out1.push(file_sink(dest1.name(), in_mode));
    out2.push( compose( compose( tolower_filter(),
                                 padding_filter('a') ),
                        file_sink(dest2.name(), in_mode) ) );
    write_data_in_chunks(out1);
    write_data_in_chunks(out2);
    out1.reset();
    out2.reset();

    {
        ifstream first(dest1.name().c_str());
        ifstream second(dest2.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed writing to a stdio_filter"
        );
    }
}

void close_composite_device()
{
    // Compose an input filter with a source
    {
        operation_sequence  seq;
        chain<input>        ch;
        ch.push(
            io::compose(
                closable_filter<input>(seq.new_operation(2)),
                closable_device<input>(seq.new_operation(1))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a bidirectional filter with a source
    {
        operation_sequence  seq;
        chain<input>        ch;
        ch.push(
            io::compose(
                closable_filter<bidirectional>(
                    seq.new_operation(2),
                    seq.new_operation(3)
                ),
                closable_device<input>(seq.new_operation(1))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a seekable filter with a source
    {
        operation_sequence  seq;
        chain<input>        ch;
        ch.push(
            io::compose(
                closable_filter<seekable>(seq.new_operation(2)),
                closable_device<input>(seq.new_operation(1))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a dual-use filter with a source
    {
        operation_sequence  seq;
        chain<input>        ch;
        operation           dummy;
        ch.push(
            io::compose(
                closable_filter<dual_use>(
                    seq.new_operation(2),
                    dummy
                ),
                closable_device<input>(seq.new_operation(1))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose an output filter with a sink
    {
        operation_sequence  seq;
        chain<output>       ch;
        ch.push(
            io::compose(
                closable_filter<output>(seq.new_operation(1)),
                closable_device<output>(seq.new_operation(2))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a bidirectional filter with a sink
    {
        operation_sequence  seq;
        chain<output>       ch;
        ch.push(
            io::compose(
                closable_filter<bidirectional>(
                    seq.new_operation(1),
                    seq.new_operation(2)
                ),
                closable_device<output>(seq.new_operation(3))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a seekable filter with a sink
    {
        operation_sequence  seq;
        chain<output>       ch;
        ch.push(
            io::compose(
                closable_filter<seekable>(seq.new_operation(1)),
                closable_device<output>(seq.new_operation(2))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a dual-use filter with a sink
    {
        operation_sequence  seq;
        chain<output>       ch;
        operation           dummy;
        ch.push(
            io::compose(
                closable_filter<dual_use>(
                    dummy,
                    seq.new_operation(1)
                ),
                closable_device<output>(seq.new_operation(2))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a bidirectional filter with a bidirectional device
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        ch.push(
            io::compose(
                closable_filter<bidirectional>(
                    seq.new_operation(2),
                    seq.new_operation(3)
                ),
                closable_device<bidirectional>(
                    seq.new_operation(1),
                    seq.new_operation(4)
                )
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a seekable filter with a seekable device
    {
        operation_sequence  seq;
        chain<seekable>     ch;
        ch.push(
            io::compose(
                closable_filter<seekable>(seq.new_operation(1)),
                closable_device<seekable>(seq.new_operation(2))
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}

void close_composite_filter()
{
    // Compose two input filters
    {
        operation_sequence  seq;
        chain<input>        ch;
        ch.push(
            io::compose(
                closable_filter<input>(seq.new_operation(3)),
                closable_filter<input>(seq.new_operation(2))
            )
        );
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a bidirectional filter with an input filter
    {
        operation_sequence  seq;
        chain<input>        ch;
        ch.push(
            io::compose(
                closable_filter<bidirectional>(
                    seq.new_operation(3),
                    seq.new_operation(4)
                ),
                closable_filter<input>(seq.new_operation(2))
            )
        );
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_MESSAGE(seq.is_success(), seq.message());
    }

    // Compose a seekable filter with an input filter
    {
        operation_sequence  seq;
        chain<input>        ch;
        ch.push(
            io::compose(
                closable_filter<seekable>(seq.new_operation(3)),
                closable_filter<input>(seq.new_operation(2))
            )
        );
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a dual-use filter with an input filter
    {
        operation_sequence  seq;
        chain<input>        ch;
        operation           dummy;
        ch.push(
            io::compose(
                closable_filter<dual_use>(
                    seq.new_operation(3),
                    dummy
                ),
                closable_filter<input>(seq.new_operation(2))
            )
        );
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose two output filters
    {
        operation_sequence  seq;
        chain<output>       ch;
        ch.push(
            io::compose(
                closable_filter<output>(seq.new_operation(1)),
                closable_filter<output>(seq.new_operation(2))
            )
        );
        ch.push(closable_device<output>(seq.new_operation(3)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a bidirectional filter with an output filter
    {
        operation_sequence  seq;
        chain<output>       ch;
        ch.push(
            io::compose(
                closable_filter<bidirectional>(
                    seq.new_operation(1),
                    seq.new_operation(2)
                ),
                closable_filter<output>(seq.new_operation(3))
            )
        );
        ch.push(closable_device<output>(seq.new_operation(4)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a seekable filter with an output filter
    {
        operation_sequence  seq;
        chain<output>       ch;
        ch.push(
            io::compose(
                closable_filter<seekable>(seq.new_operation(1)),
                closable_filter<output>(seq.new_operation(2))
            )
        );
        ch.push(closable_device<output>(seq.new_operation(3)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose a dual-use filter with an output filter
    {
        operation_sequence  seq;
        chain<output>       ch;
        operation           dummy;
        ch.push(
            io::compose(
                closable_filter<dual_use>(
                    dummy,
                    seq.new_operation(1)
                ),
                closable_filter<output>(seq.new_operation(2))
            )
        );
        ch.push(closable_device<output>(seq.new_operation(3)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose two bidirectional filters
    {
        operation_sequence    seq;
        chain<bidirectional>  ch;
        ch.push(
            io::compose(
                closable_filter<bidirectional>(
                    seq.new_operation(3),
                    seq.new_operation(4)
                ),
                closable_filter<bidirectional>(
                    seq.new_operation(2),
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

    // Compose two seekable filters
    {
        operation_sequence  seq;
        chain<seekable>     ch;
        ch.push(
            io::compose(
                closable_filter<seekable>(seq.new_operation(1)),
                closable_filter<seekable>(seq.new_operation(2))
            )
        );
        ch.push(closable_device<seekable>(seq.new_operation(3)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose two dual-use filters for input
    {
        operation_sequence  seq;
        chain<input>        ch;
        operation           dummy;
        ch.push(
            io::compose(
                closable_filter<dual_use>(
                    seq.new_operation(3),
                    dummy
                ),
                closable_filter<dual_use>(
                    seq.new_operation(2),
                    dummy
                )
            )
        );
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Compose two dual-use filters for output
    {
        operation_sequence  seq;
        chain<output>       ch;
        operation           dummy;
        ch.push(
            io::compose(
                closable_filter<dual_use>(
                    dummy,
                    seq.new_operation(1)
                ),
                closable_filter<dual_use>(
                    dummy,
                    seq.new_operation(2)
                )
            )
        );
        ch.push(closable_device<output>(seq.new_operation(3)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("line_filter test");
    test->add(BOOST_TEST_CASE(&read_composite));
    test->add(BOOST_TEST_CASE(&write_composite));
    test->add(BOOST_TEST_CASE(&close_composite_device));
    test->add(BOOST_TEST_CASE(&close_composite_filter));
    return test;
}

#include <boost/iostreams/detail/config/enable_warnings.hpp> // BCC 5.x.
