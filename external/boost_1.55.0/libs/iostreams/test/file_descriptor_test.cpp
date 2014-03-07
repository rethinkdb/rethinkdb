// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <fstream>
#include <fcntl.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"
#include "detail/file_handle.hpp"

using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
namespace boost_ios = boost::iostreams;
using std::ifstream;
using boost::unit_test::test_suite;   

void file_descriptor_test()
{

    typedef stream<file_descriptor_source> fdistream;
    typedef stream<file_descriptor_sink>   fdostream;
    typedef stream<file_descriptor>        fdstream;

    test_file  test1;       
    test_file  test2;       
                    
    //--------------Test file_descriptor_source-------------------------------//

    {
        fdistream  first(file_descriptor_source(test1.name()), 0);
        ifstream   second(test2.name().c_str());
        BOOST_CHECK(first->is_open());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chars(first, second),
            "failed reading from file_descriptor_source in chars with no buffer"
        );
        first->close();
        BOOST_CHECK(!first->is_open());
    }

    {
        fdistream  first(file_descriptor_source(test1.name()), 0);
        ifstream   second(test2.name().c_str());
        BOOST_CHECK(first->is_open());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed reading from file_descriptor_source in chunks with no buffer"
        );
        first->close();
        BOOST_CHECK(!first->is_open());
    }

    {
        file_descriptor_source  file(test1.name());
        fdistream               first(file);
        ifstream                second(test2.name().c_str());
        BOOST_CHECK(first->is_open());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chars(first, second),
            "failed reading from file_descriptor_source in chars with buffer"
        );
        first->close();
        BOOST_CHECK(!first->is_open());
    }

    {
        file_descriptor_source  file(test1.name());
        fdistream               first(file);
        ifstream                second(test2.name().c_str());
        BOOST_CHECK(first->is_open());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed reading from file_descriptor_source in chunks with buffer"
        );
        first->close();
        BOOST_CHECK(!first->is_open());
    }

    // test illegal flag combinations
    {
        BOOST_CHECK_THROW(
            file_descriptor_source(test1.name(),
                BOOST_IOS::app),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor_source(test1.name(),
                BOOST_IOS::trunc),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor_source(test1.name(),
                BOOST_IOS::app | BOOST_IOS::trunc),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor_source(test1.name(),
                BOOST_IOS::out),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor_source(test1.name(),
                BOOST_IOS::out | BOOST_IOS::app),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor_source(test1.name(),
                BOOST_IOS::out | BOOST_IOS::trunc),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor_source(test1.name(),
                BOOST_IOS::out | BOOST_IOS::app | BOOST_IOS::trunc),
            BOOST_IOSTREAMS_FAILURE);
    }

    //--------------Test file_descriptor_sink---------------------------------//

    {
        temp_file             temp;
        file_descriptor_sink  file(temp.name(), BOOST_IOS::trunc);
        fdostream             out(file, 0);
        BOOST_CHECK(out->is_open());
        write_data_in_chars(out);
        out.close();
        BOOST_CHECK_MESSAGE(
            compare_files(test1.name(), temp.name()),
            "failed writing to file_descriptor_sink in chars with no buffer"
        );
        file.close();
        BOOST_CHECK(!file.is_open());
    }

    {
        temp_file             temp;
        file_descriptor_sink  file(temp.name(), BOOST_IOS::trunc);
        fdostream             out(file, 0);
        BOOST_CHECK(out->is_open());
        write_data_in_chunks(out);
        out.close();
        BOOST_CHECK_MESSAGE(
            compare_files(test1.name(), temp.name()),
            "failed writing to file_descriptor_sink in chunks with no buffer"
        );
        file.close();
        BOOST_CHECK(!file.is_open());
    }

    {
        temp_file             temp;
        file_descriptor_sink  file(temp.name(), BOOST_IOS::trunc);
        fdostream             out(file);
        BOOST_CHECK(out->is_open());
        write_data_in_chars(out);
        out.close();
        BOOST_CHECK_MESSAGE(
            compare_files(test1.name(), temp.name()),
            "failed writing to file_descriptor_sink in chars with buffer"
        );
        file.close();
        BOOST_CHECK(!file.is_open());
    }

    {
        temp_file             temp;
        file_descriptor_sink  file(temp.name(), BOOST_IOS::trunc);
        fdostream             out(file);
        BOOST_CHECK(out->is_open());
        write_data_in_chunks(out);
        out.close();
        BOOST_CHECK_MESSAGE(
            compare_files(test1.name(), temp.name()),
            "failed writing to file_descriptor_sink in chunks with buffer"
        );
        file.close();
        BOOST_CHECK(!file.is_open());
    }
                                                    
    {
        temp_file             temp;
        // set up the tests
        {
            file_descriptor_sink  file(temp.name(), BOOST_IOS::trunc);
            fdostream             out(file);
            write_data_in_chunks(out);
            out.close();
            file.close();
        }
        // test std::ios_base::app
        {
            file_descriptor_sink  file(temp.name(), BOOST_IOS::app);
            fdostream             out(file);
            BOOST_CHECK(out->is_open());
            write_data_in_chars(out);
            out.close();
            std::string expected(narrow_data());
            expected += narrow_data();
            BOOST_CHECK_MESSAGE(
                compare_container_and_file(expected, temp.name()),
                "failed writing to file_descriptor_sink in append mode"
            );
            file.close();
            BOOST_CHECK(!file.is_open());
        }
        // test std::ios_base::trunc
        {
            file_descriptor_sink  file(temp.name(), BOOST_IOS::trunc);
            fdostream             out(file);
            BOOST_CHECK(out->is_open());
            write_data_in_chars(out);
            out.close();
            BOOST_CHECK_MESSAGE(
                compare_files(test1.name(), temp.name()),
                "failed writing to file_descriptor_sink in trunc mode"
            );
            file.close();
            BOOST_CHECK(!file.is_open());
        }
        
        // test illegal flag combinations
        {
            BOOST_CHECK_THROW(
                file_descriptor_sink(temp.name(),
                    BOOST_IOS::trunc | BOOST_IOS::app),
                BOOST_IOSTREAMS_FAILURE);
            BOOST_CHECK_THROW(
                file_descriptor_sink(temp.name(),
                    BOOST_IOS::in),
                BOOST_IOSTREAMS_FAILURE);
            BOOST_CHECK_THROW(
                file_descriptor_sink(temp.name(),
                    BOOST_IOS::in | BOOST_IOS::app),
                BOOST_IOSTREAMS_FAILURE);
            BOOST_CHECK_THROW(
                file_descriptor_sink(temp.name(),
                    BOOST_IOS::in | BOOST_IOS::trunc),
                BOOST_IOSTREAMS_FAILURE);
            BOOST_CHECK_THROW(
                file_descriptor_sink(temp.name(),
                    BOOST_IOS::in | BOOST_IOS::trunc | BOOST_IOS::app),
                BOOST_IOSTREAMS_FAILURE);
        }
    }

    //--Test seeking with file_descriptor_source and file_descriptor_sink-----//

    test_file test3;
    {
        file_descriptor_sink  sink(test3.name());
        fdostream             out(sink);
        BOOST_CHECK(out->is_open());
        BOOST_CHECK_MESSAGE(
            test_output_seekable(out),
            "failed seeking within a file_descriptor_sink"
        );
        out->close();
        BOOST_CHECK(!out->is_open());

        file_descriptor_source  source(test3.name());
        fdistream               in(source);
        BOOST_CHECK(in->is_open());
        BOOST_CHECK_MESSAGE(
            test_input_seekable(in),
            "failed seeking within a file_descriptor_source"
        );
        in->close();
        BOOST_CHECK(!in->is_open());
    }

    //--------------Test file_descriptor--------------------------------------//

    {
        temp_file                  temp;
        file_descriptor            file( temp.name(),
                                         BOOST_IOS::in | 
                                         BOOST_IOS::out |
                                         BOOST_IOS::trunc | 
                                         BOOST_IOS::binary );
        fdstream                   io(file, BUFSIZ);
        BOOST_CHECK_MESSAGE(
            test_seekable_in_chars(io),
            "failed seeking within a file_descriptor, in chars"
        );
    }

    {
        temp_file                  temp;
        file_descriptor            file( temp.name(),
                                         BOOST_IOS::in | 
                                         BOOST_IOS::out |
                                         BOOST_IOS::trunc | 
                                         BOOST_IOS::binary );
        fdstream                   io(file, BUFSIZ);
        BOOST_CHECK_MESSAGE(
            test_seekable_in_chunks(io),
            "failed seeking within a file_descriptor, in chunks"
        );
    }
    
    //--------------Test read-only file_descriptor----------------------------//
    
    {
        fdstream   first(file_descriptor(test1.name(), BOOST_IOS::in), 0);
        ifstream   second(test2.name().c_str());
        BOOST_CHECK(first->is_open());
        write_data_in_chars(first);
        BOOST_CHECK(first.fail());
        first.clear();
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chars(first, second),
            "failed reading from file_descriptor in chars with no buffer"
        );
        first->close();
        BOOST_CHECK(!first->is_open());
    }

    {
        fdstream   first(file_descriptor(test1.name(), BOOST_IOS::in), 0);
        ifstream   second(test2.name().c_str());
        BOOST_CHECK(first->is_open());
        write_data_in_chunks(first);
        BOOST_CHECK(first.fail());
        first.clear();
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed reading from file_descriptor in chunks with no buffer"
        );
        first->close();
        BOOST_CHECK(!first->is_open());
    }

    {
        file_descriptor         file(test1.name(), BOOST_IOS::in);
        fdstream                first(file);
        ifstream                second(test2.name().c_str());
        BOOST_CHECK(first->is_open());
        write_data_in_chars(first);
        BOOST_CHECK(first.fail());
        first.clear();
        first.seekg(0, BOOST_IOS::beg);
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chars(first, second),
            "failed reading from file_descriptor in chars with buffer"
        );
        first->close();
        BOOST_CHECK(!first->is_open());
    }

    {
        file_descriptor         file(test1.name(), BOOST_IOS::in);
        fdstream                first(file);
        ifstream                second(test2.name().c_str());
        BOOST_CHECK(first->is_open());
        write_data_in_chunks(first);
        BOOST_CHECK(first.fail());
        first.clear();
        first.seekg(0, BOOST_IOS::beg);
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed reading from file_descriptor in chunks with buffer"
        );
        first->close();
        BOOST_CHECK(!first->is_open());
    }

    //--------------Test write-only file_descriptor---------------------------//
    {
        temp_file             temp;
        file_descriptor       file( temp.name(),
                                    BOOST_IOS::out |
                                    BOOST_IOS::trunc );
        fdstream              out(file, 0);
        BOOST_CHECK(out->is_open());
        out.get();
        BOOST_CHECK(out.fail());
        out.clear();
        write_data_in_chars(out);
        out.seekg(0, BOOST_IOS::beg);
        out.get();
        BOOST_CHECK(out.fail());
        out.clear();
        out.close();
        BOOST_CHECK_MESSAGE(
            compare_files(test1.name(), temp.name()),
            "failed writing to file_descriptor in chars with no buffer"
        );
        file.close();
        BOOST_CHECK(!file.is_open());
    }

    {
        temp_file             temp;
        file_descriptor       file( temp.name(),
                                    BOOST_IOS::out |
                                    BOOST_IOS::trunc );
        fdstream              out(file, 0);
        BOOST_CHECK(out->is_open());
        out.get();
        BOOST_CHECK(out.fail());
        out.clear();
        write_data_in_chunks(out);
        out.seekg(0, BOOST_IOS::beg);
        out.get();
        BOOST_CHECK(out.fail());
        out.clear();
        out.close();
        BOOST_CHECK_MESSAGE(
            compare_files(test1.name(), temp.name()),
            "failed writing to file_descriptor_sink in chunks with no buffer"
        );
        file.close();
        BOOST_CHECK(!file.is_open());
    }

    {
        temp_file             temp;
        file_descriptor       file( temp.name(),
                                    BOOST_IOS::out |
                                    BOOST_IOS::trunc );
        fdstream              out(file);
        BOOST_CHECK(out->is_open());
        out.get();
        BOOST_CHECK(out.fail());
        out.clear();
        write_data_in_chars(out);
        out.seekg(0, BOOST_IOS::beg);
        out.get();
        BOOST_CHECK(out.fail());
        out.clear();
        out.close();
        BOOST_CHECK_MESSAGE(
            compare_files(test1.name(), temp.name()),
            "failed writing to file_descriptor_sink in chars with buffer"
        );
        file.close();
        BOOST_CHECK(!file.is_open());
    }

    {
        temp_file             temp;
        file_descriptor       file( temp.name(),
                                    BOOST_IOS::out |
                                    BOOST_IOS::trunc );
        fdstream              out(file);
        BOOST_CHECK(out->is_open());
        out.get();
        BOOST_CHECK(out.fail());
        out.clear();
        write_data_in_chunks(out);
        out.seekg(0, BOOST_IOS::beg);
        out.get();
        BOOST_CHECK(out.fail());
        out.clear();
        out.close();
        BOOST_CHECK_MESSAGE(
            compare_files(test1.name(), temp.name()),
            "failed writing to file_descriptor_sink in chunks with buffer"
        );
        file.close();
        BOOST_CHECK(!file.is_open());
    }

    // test illegal flag combinations
    {
        BOOST_CHECK_THROW(
            file_descriptor(test1.name(),
                BOOST_IOS::openmode(0)),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor(test1.name(),
                BOOST_IOS::app),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor(test1.name(),
                BOOST_IOS::trunc),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor(test1.name(),
                BOOST_IOS::app | BOOST_IOS::trunc),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor(test1.name(),
                BOOST_IOS::in | BOOST_IOS::app),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor(test1.name(),
                BOOST_IOS::in | BOOST_IOS::trunc),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor(test1.name(),
                BOOST_IOS::in | BOOST_IOS::app | BOOST_IOS::trunc),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor(test1.name(),
                BOOST_IOS::out | BOOST_IOS::app | BOOST_IOS::trunc),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor(test1.name(),
                BOOST_IOS::in | BOOST_IOS::out | BOOST_IOS::app),
            BOOST_IOSTREAMS_FAILURE);
        BOOST_CHECK_THROW(
            file_descriptor(test1.name(),
                BOOST_IOS::in |
                BOOST_IOS::out |
                BOOST_IOS::app |
                BOOST_IOS::trunc),
            BOOST_IOSTREAMS_FAILURE);
    }
}

template <class FileDescriptor>
void file_handle_test_impl(FileDescriptor*)
{
    test_file  test1;       
    test_file  test2;       

    {
        boost_ios::detail::file_handle handle = open_file_handle(test1.name());
        {
            FileDescriptor device1(handle, boost_ios::never_close_handle);
            BOOST_CHECK(device1.handle() == handle);
        }
        BOOST_CHECK_HANDLE_OPEN(handle);
        close_file_handle(handle);
    }

    {
        boost_ios::detail::file_handle handle = open_file_handle(test1.name());
        {
            FileDescriptor device1(handle, boost_ios::close_handle);
            BOOST_CHECK(device1.handle() == handle);
        }
        BOOST_CHECK_HANDLE_CLOSED(handle);
    }

    {
        boost_ios::detail::file_handle handle = open_file_handle(test1.name());
        FileDescriptor device1(handle, boost_ios::never_close_handle);
        BOOST_CHECK(device1.handle() == handle);
        device1.close();
        BOOST_CHECK(!device1.is_open());
        BOOST_CHECK_HANDLE_OPEN(handle);
        close_file_handle(handle);
    }

    {
        boost_ios::detail::file_handle handle = open_file_handle(test1.name());
        FileDescriptor device1(handle, boost_ios::close_handle);
        BOOST_CHECK(device1.handle() == handle);
        device1.close();
        BOOST_CHECK(!device1.is_open());
        BOOST_CHECK_HANDLE_CLOSED(handle);
    }

    {
        boost_ios::detail::file_handle handle1 = open_file_handle(test1.name());
        boost_ios::detail::file_handle handle2 = open_file_handle(test2.name());
        {
            FileDescriptor device1(handle1, boost_ios::never_close_handle);
            BOOST_CHECK(device1.handle() == handle1);
            device1.open(handle2, boost_ios::never_close_handle);
            BOOST_CHECK(device1.handle() == handle2);
        }
        BOOST_CHECK_HANDLE_OPEN(handle1);
        BOOST_CHECK_HANDLE_OPEN(handle2);
        close_file_handle(handle1);
        close_file_handle(handle2);
    }

    {
        boost_ios::detail::file_handle handle1 = open_file_handle(test1.name());
        boost_ios::detail::file_handle handle2 = open_file_handle(test2.name());
        {
            FileDescriptor device1(handle1, boost_ios::close_handle);
            BOOST_CHECK(device1.handle() == handle1);
            device1.open(handle2, boost_ios::close_handle);
            BOOST_CHECK(device1.handle() == handle2);
            BOOST_CHECK_HANDLE_CLOSED(handle1);
            BOOST_CHECK_HANDLE_OPEN(handle2);
        }
        BOOST_CHECK_HANDLE_CLOSED(handle1);
        BOOST_CHECK_HANDLE_CLOSED(handle2);
    }

    {
        boost_ios::detail::file_handle handle1 = open_file_handle(test1.name());
        boost_ios::detail::file_handle handle2 = open_file_handle(test2.name());
        {
            FileDescriptor device1(handle1, boost_ios::close_handle);
            BOOST_CHECK(device1.handle() == handle1);
            device1.open(handle2, boost_ios::never_close_handle);
            BOOST_CHECK(device1.handle() == handle2);
            BOOST_CHECK_HANDLE_CLOSED(handle1);
            BOOST_CHECK_HANDLE_OPEN(handle2);
        }
        BOOST_CHECK_HANDLE_CLOSED(handle1);
        BOOST_CHECK_HANDLE_OPEN(handle2);
        close_file_handle(handle2);
    }

    {
        boost_ios::detail::file_handle handle = open_file_handle(test1.name());
        {
            FileDescriptor device1;
            BOOST_CHECK(!device1.is_open());
            device1.open(handle, boost_ios::never_close_handle);
            BOOST_CHECK(device1.handle() == handle);
            BOOST_CHECK_HANDLE_OPEN(handle);
        }
        BOOST_CHECK_HANDLE_OPEN(handle);
        close_file_handle(handle);
    }

    {
        boost_ios::detail::file_handle handle = open_file_handle(test1.name());
        {
            FileDescriptor device1;
            BOOST_CHECK(!device1.is_open());
            device1.open(handle, boost_ios::close_handle);
            BOOST_CHECK(device1.handle() == handle);
            BOOST_CHECK_HANDLE_OPEN(handle);
        }
        BOOST_CHECK_HANDLE_CLOSED(handle);
    }
}

void file_handle_test()
{
    file_handle_test_impl((boost_ios::file_descriptor*) 0);
    file_handle_test_impl((boost_ios::file_descriptor_source*) 0);
    file_handle_test_impl((boost_ios::file_descriptor_sink*) 0);
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("file_descriptor test");
    test->add(BOOST_TEST_CASE(&file_descriptor_test));
    test->add(BOOST_TEST_CASE(&file_handle_test));
    return test;
}
