// (C) Copyright Jorge Lodos 2008
// (C) Copyright Jonathan Turkanis 2004
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// This is the original (boost 1.34) boost::iostream test for the mapped files with the
// following modifications:
// 1. The namespace for the mapped file was changed to seglib::filemap.
// 2. Added test for privately mapped files.
// 3. The test test_writeable was added for mapped files.
// 4. The test test_resizeable was added for mapped files.
//

#include <fstream>
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem/path.hpp>
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

// Code generation bugs cause tests to fail with global optimization.
#if BOOST_WORKAROUND(BOOST_MSVC, < 1300)
# pragma optimize("g", off)
#endif

namespace boost { namespace iostreams { namespace test {

bool test_writeable(mapped_file& mf)
{
    // Test writing
    for (int i = 0; i < data_reps; ++i) {
        memcpy(mf.data(), narrow_data(), chunk_size);
        char buf[chunk_size];
        memcpy(buf, mf.const_data(), chunk_size);
        if (strncmp(buf, narrow_data(), chunk_size) != 0)
            return false;
        memset(mf.data(), 0, chunk_size);
    }
    return true;
}

bool test_resizeable(mapped_file& mf)
{
    // Test resizing
    mapped_file::size_type size = mf.size();
    if (size == 0)
        return false;
    mf.resize(size/2);
    if (mf.size() != size/2)
        return false;
    mf.resize(size);
    if (mf.size() != size)
        return false;
    return true;
}

} } } // End namespaces test, iostreams, boost.

void mapped_file_test()
{
    using namespace boost::iostreams;
    BOOST_TEST_MESSAGE("about to begin");

    //--------------Reading from a mapped_file_source-------------------------//

    {
        // Note: the ifstream second is placed in a nested scope because 
        // closing and reopening a single ifstream failed for CW 9.4 on Windows.

        // Test reading from a stream based on a mapped_file_source,
        // in chars.
        boost::iostreams::test::test_file test1, test2;
        boost::iostreams::stream<mapped_file_source> first(test1.name());
        {
            std::ifstream second( test2.name().c_str(), 
                             BOOST_IOS::in | BOOST_IOS::binary );
            BOOST_CHECK_MESSAGE(
                boost::iostreams::test::compare_streams_in_chars(first, second),
                "failed reading from stream<mapped_file_source> in chars"
            );

            BOOST_TEST_MESSAGE(
                "done reading from stream<mapped_file_source> in chars"
            );
        }
        first.close();

        // Test reading from a stream based on a mapped_file_source,
        // in chunks. (Also tests reopening the stream.)
        first.open(mapped_file_source(test1.name()));
        {
            std::ifstream second( test2.name().c_str(), 
                             BOOST_IOS::in | BOOST_IOS::binary );
            BOOST_CHECK_MESSAGE(
                boost::iostreams::test::compare_streams_in_chunks(first, second),
                "failed reading from stream<mapped_file_source> in chunks"
            );

            BOOST_TEST_MESSAGE(
                "done reading from stream<mapped_file_source> in chunks"
            );
        }
    }

    //--------------Writing to a mapped_file_sink-----------------------------//

    {
        // Test writing to a stream based on a mapped_file_sink, in 
        // chars.
        boost::iostreams::test::uppercase_file  first, second; // Will overwrite these.
        boost::iostreams::test::test_file       test;

        boost::iostreams::stream<mapped_file_sink> out;
        out.open(mapped_file_sink(first.name()));
        boost::iostreams::test::write_data_in_chars(out);
        out.close();
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::compare_files(first.name(), test.name()),
            "failed writing to stream<mapped_file_sink> in chars"
        );

        BOOST_TEST_MESSAGE(
            "done writing to stream<mapped_file_source> in chars"
        );

        // Test writing to a stream based on a mapped_file_sink, in 
        // chunks. (Also tests reopening the stream.)
        out.open(mapped_file_sink(second.name()));
        boost::iostreams::test::write_data_in_chunks(out);
        out.close();
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::compare_files(second.name(), test.name()),
            "failed writing to stream<mapped_file_sink> in chunks"
        );

        BOOST_TEST_MESSAGE(
            "done writing to stream<mapped_file_source> in chunks"
        );
    }

    //--------------Writing to a newly created file---------------------------//

    {
        // Test writing to a newly created mapped file.
        boost::iostreams::test::temp_file  first, second;
        boost::iostreams::test::test_file  test;

        mapped_file_params p(first.name());
        p.new_file_size = boost::iostreams::test::data_reps * boost::iostreams::test::data_length();
        boost::iostreams::stream<mapped_file_sink> out;
        out.open(mapped_file_sink(p));
        boost::iostreams::test::write_data_in_chars(out);
        out.close();
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::compare_files(first.name(), test.name()),
            "failed writing to newly created mapped file in chars"
        );

        
        // Test writing to a newly created mapped file. 
        // (Also tests reopening the stream.)
        p.path = second.name();
        out.open(mapped_file_sink(p));
        boost::iostreams::test::write_data_in_chunks(out);
        out.close();
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::compare_files(second.name(), test.name()),
            "failed writing to newly created mapped file in chunks"
        );
    }

    //--------------Writing to a pre-existing file---------------------------//
    {
        // Test for Bug #3953 - writing to a pre-existing mapped file.
        boost::iostreams::test::test_file  first, test;

        mapped_file_params p(first.name());
        p.new_file_size = boost::iostreams::test::data_reps * boost::iostreams::test::data_length();
        boost::iostreams::stream<mapped_file_sink> out;
        out.open(mapped_file_sink(p));
        boost::iostreams::test::write_data_in_chars(out);
        out.close();
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::compare_files(first.name(), test.name()),
            "failed writing to pre-existing mapped file in chars"
        );
    }
    
    //--------------Random access with a mapped_file--------------------------//

    {
        // Test reading, writing and seeking within a stream based on a 
        // mapped_file, in chars.
        boost::iostreams::test::test_file test;
        boost::iostreams::stream<mapped_file> io;
        io.open(mapped_file(test.name()));
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::test_seekable_in_chars(io),
            "failed seeking within stream<mapped_file> in chars"
        );

        BOOST_TEST_MESSAGE(
            "done seeking within stream<mapped_file> in chars"
        );

        io.close();

        // Test reading, writing and seeking within a stream based on a 
        // mapped_file, in chunks. (Also tests reopening the 
        // stream.)
        io.open(mapped_file(test.name()));
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::test_seekable_in_chunks(io),
            "failed seeking within stream<mapped_file> in chunks"
        );

        BOOST_TEST_MESSAGE(
            "done seeking within stream<mapped_file> in chunks"
        );
    }

    //--------------Resizing a mapped_file------------------------------------//

    {
        // Test resizing a mapped_file.
        boost::iostreams::test::test_file test;
        mapped_file mf;
        mf.open(test.name());
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::test_resizeable(mf),
            "failed resizing a mapped_file"
        );
        
        BOOST_TEST_MESSAGE(
            "done resizing a mapped_file"
        );
    }

    //--------------Random access with a private mapped_file------------------//

    {
        // Use 2 copies of the file to compare later
        boost::iostreams::test::test_file orig, copy;
        
        // Test reading and writing within a mapped_file. 
        // Since the file is privately mapped, it should remain
        // unchanged after writing when opened in readonly mode.
        mapped_file mf;
        mf.open(orig.name(), mapped_file::priv);
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::test_writeable(mf),
            "failed seeking within private mapped_file"
        );
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::compare_files(orig.name(), copy.name()),
            "failed writing to private mapped_file"
        );
        
        BOOST_TEST_MESSAGE(
            "done seeking within private mapped_file"
        );
        
        mf.close();

        // Test reopening the mapped file.
        mf.open(orig.name(), mapped_file::priv);
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::test_writeable(mf),
            "failed reopening private mapped_file"
        );
        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::compare_files(orig.name(), copy.name()),
            "failed writing to reopened private mapped_file"
        );
        
        BOOST_TEST_MESSAGE(
            "done reopening private mapped_file"
        );
    }

    //-------------Check creating opening mapped_file with char*-------------//
    
    {
        boost::iostreams::test::test_file orig;
        char name[50];
        std::strcpy(name, orig.name().c_str());
        
        mapped_file mf((char*) name);

        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::test_writeable(mf),
            "failed seeking within private mapped_file"
        );

        mf.close();
    }

    //---------Check creating opening mapped_file with filesystem3 path------//
    {
        boost::iostreams::test::test_file orig;
        
        mapped_file mf(boost::filesystem::path(orig.name()));

        BOOST_CHECK_MESSAGE(
            boost::iostreams::test::test_writeable(mf),
            "failed seeking within private mapped_file"
        );

        mf.close();
    }
}

#if BOOST_WORKAROUND(BOOST_MSVC, < 1300)
# pragma optimize("", on)
#endif

boost::unit_test::test_suite* init_unit_test_suite(int, char* []) 
{
    boost::unit_test::test_suite* test = BOOST_TEST_SUITE("mapped_file test");
    test->add(BOOST_TEST_CASE(&mapped_file_test));
    return test;
}
