// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/detail/config/wide_streams.hpp>
#ifdef BOOST_IOSTREAMS_NO_WIDE_STREAMS
# error wide streams not supported on this platform
#endif

#include <algorithm>         // equal.
#include <locale>
#include <string>
#include <boost/config.hpp>  // BOOST_DEDUCED_TYPENAME.
#include <boost/iostreams/code_converter.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/detail/add_facet.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/detail/config/windows_posix.hpp>
#include <boost/iostreams/device/file.hpp>
#if !defined(__COMO__) || !defined(BOOST_COMO_STRICT)
# if defined(BOOST_IOSTREAMS_NO_LIB) || defined(BOOST_ALL_NO_LIB)
#  include "../src/file_descriptor.cpp"
# else
#  include <boost/iostreams/device/file_descriptor.hpp>
# endif
#endif
#include <boost/iostreams/stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/closable.hpp"
#include "detail/operation_sequence.hpp"
#include "detail/temp_file.hpp"

    // Include codevct facets

#include "detail/null_padded_codecvt.hpp"
#include "detail/utf8_codecvt_facet.hpp"
#ifdef BOOST_IOSTREAMS_USE_DINKUM_COREX
# include <codecvt/8859_1>
# include <codecvt/8859_10>
# include <codecvt/8859_13>
# include <codecvt/8859_14>
# include <codecvt/8859_15>
# include <codecvt/8859_16>
# include <codecvt/8859_2>
# include <codecvt/8859_3>
# include <codecvt/8859_4>
# include <codecvt/8859_5>
# include <codecvt/8859_6>
# include <codecvt/8859_7>
# include <codecvt/8859_8>
# include <codecvt/8859_9>
# include <codecvt/baltic>
# include <codecvt/big5>
# include <codecvt/cp037>
# include <codecvt/cp1006>
# include <codecvt/cp1026>
# include <codecvt/cp1250>
# include <codecvt/cp1251>
# include <codecvt/cp1252>
# include <codecvt/cp1253>
# include <codecvt/cp1254>
# include <codecvt/cp1255>
# include <codecvt/cp1256>
# include <codecvt/cp1257>
# include <codecvt/cp1258>
# include <codecvt/cp424>
# include <codecvt/cp437>
# include <codecvt/cp500>
# include <codecvt/cp737>
# include <codecvt/cp775>
# include <codecvt/cp850>
# include <codecvt/cp852>
# include <codecvt/cp855>
# include <codecvt/cp856>
# include <codecvt/cp857>
# include <codecvt/cp860>
# include <codecvt/cp861>
# include <codecvt/cp862>
# include <codecvt/cp863>
# include <codecvt/cp864>
# include <codecvt/cp865>
# include <codecvt/cp866>
# include <codecvt/cp869>
# include <codecvt/cp874>
# include <codecvt/cp875>
# include <codecvt/cp932>
# include <codecvt/cp936>
# include <codecvt/cp949>
# include <codecvt/cp950>
# include <codecvt/cyrillic>
# include <codecvt/ebcdic>
# include <codecvt/euc>
# include <codecvt/euc_0208>
# include <codecvt/gb12345>
# include <codecvt/gb2312>
# include <codecvt/greek>
# include <codecvt/iceland>
# include <codecvt/jis>
# include <codecvt/jis_0208>
# include <codecvt/jis0201>
# include <codecvt/ksc5601>
# include <codecvt/latin2>
# include <codecvt/one_one>
# include <codecvt/roman>
# include <codecvt/sjis>
# include <codecvt/sjis_0208>
# include <codecvt/turkish>
# include <codecvt/utf16>
# include <codecvt/utf8>
# include <codecvt/utf8_utf16>
# include <codecvt/xjis>
#endif // #ifdef BOOST_IOSTREAMS_USE_DINKUM_COREX]

#include <iostream>

using namespace std;
using namespace boost::iostreams;
using namespace boost::iostreams::detail;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;
namespace io = boost::iostreams;

const int max_length = 30;
const unsigned int pattern_length = 100;
const unsigned int pattern_reps = 100;

template<typename Codecvt>
bool valid_char(typename codecvt_intern<Codecvt>::type c)
{
    typedef typename codecvt_state<Codecvt>::type   state_type;
    typedef typename codecvt_intern<Codecvt>::type  intern_type;
    Codecvt             cvt;
    state_type          state = state_type();
    const intern_type*  nint;
    char*               next;
    char                buf[max_length];

    return cvt.out( state, &c, &c + 1, nint,
                    buf, buf + max_length, next )
           ==
           codecvt_base::ok;
}

template<typename Codecvt>
basic_string<
    BOOST_DEDUCED_TYPENAME
    codecvt_intern<Codecvt>::type
>
test_string()
{
    typedef typename codecvt_intern<Codecvt>::type intern_type;
    std::basic_string<intern_type> pattern, result;
    for (intern_type c = 255; pattern.size() < pattern_length; --c)
        if (valid_char<Codecvt>(c))
            pattern += c;
    result.reserve(pattern.size() * pattern_reps);
    for (unsigned int w = 0; w < pattern_reps; ++w)
        result += pattern;
    return result;
}

// Como can't compile file_descriptor.cpp in strict mode; this failure
// is detected by file_descriptor_test.cpp.
#if !defined(__COMO__) || !defined(BOOST_COMO_STRICT)
    typedef io::file_descriptor_source  classic_file_source;
    typedef io::file_descriptor_sink    classic_file_sink;
#else
    struct classic_file_source : io::source {
        classic_file_source(const std::string& path)
            : file_(new filebuf)
        {
            file_->pubimbue(locale::classic());
            file_->open(path.c_str(), BOOST_IOS::in | BOOST_IOS::binary);
        }
        streamsize read(char* s, streamsize n) { return file_->sgetn(s, n); }
        boost::shared_ptr<filebuf> file_;
    };

    struct classic_file_sink : io::sink  {
        classic_file_sink(const std::string& path)
            : file_(new filebuf)
        {
            file_->pubimbue(locale::classic());
            file_->open(path.c_str(), BOOST_IOS::out | BOOST_IOS::binary);
        }
        streamsize write(const char* s, streamsize n) { return file_->sputn(s, n); }
        boost::shared_ptr<filebuf> file_;
    };
#endif

template<typename Codecvt>
bool codecvt_test1()
{
    typedef basic_string<
                BOOST_DEDUCED_TYPENAME
                codecvt_intern<Codecvt>::type
            >                                             string_type;
    typedef code_converter<classic_file_source, Codecvt>  wide_file_source;
    typedef code_converter<classic_file_sink, Codecvt>    wide_file_sink;

    BOOST_CHECK(Codecvt().max_length() <= max_length);
    temp_file                 temp;
    string_type               test = test_string<Codecvt>();
    stream<wide_file_sink>    out(temp.name());
    out.write(test.data(), static_cast<streamsize>(test.size()));
    out.close();

    stream<wide_file_source>  in(temp.name());
    string_type               test2;
    io::copy(in, io::back_inserter(test2));

    return test == test2;
}

template<typename Codecvt>
bool codecvt_test2()
{
    typedef basic_string<
                BOOST_DEDUCED_TYPENAME
                codecvt_intern<Codecvt>::type
            >                                    string_type;
    typedef code_converter<classic_file_source>  wide_file_source;
    typedef code_converter<classic_file_sink>    wide_file_sink;

    // Set global locale.
    locale loc = add_facet(locale(), new Codecvt);
    locale::global(loc);

    temp_file                 temp;
    string_type               test = test_string<Codecvt>();
    stream<wide_file_sink>    out(temp.name());
    out.write(test.data(), static_cast<streamsize>(test.size()));
    out.close();

    stream<wide_file_source>  in(temp.name());
    string_type               test2;
    io::copy(in, io::back_inserter(test2));

    return test == test2;
}

template<typename Codecvt>
bool codecvt_test()
{
    return codecvt_test1<Codecvt>() && codecvt_test2<Codecvt>();
}

void code_converter_test()
{
    BOOST_CHECK((codecvt_test<utf8_codecvt_facet<wchar_t, char> >()));
    BOOST_CHECK(codecvt_test<null_padded_codecvt>());
    BOOST_CHECK(codecvt_test<stateless_null_padded_codecvt>());
#ifdef BOOST_IOSTREAMS_USE_DINKUM_COREX
    using namespace Dinkum::conversions;
    BOOST_CHECK(codecvt_test< codecvt_8859_1<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_10<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_13<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_14<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_15<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_16<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_2<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_3<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_4<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_5<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_6<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_7<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_8<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_8859_9<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_baltic<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_big5<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp037<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1006<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1026<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1250<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1251<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1252<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1253<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1254<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1255<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1256<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1257<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp1258<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp424<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp437<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp500<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp737<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp775<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp850<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp852<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp855<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp856<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp857<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp860<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp861<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp862<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp863<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp864<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp865<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp866<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp869<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp874<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp875<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp932<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp936<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp949<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cp950<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_cyrillic<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_ebcdic<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_euc<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_euc_0208<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_gb12345<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_gb2312<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_greek<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_iceland<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_jis<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_jis_0208<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_jis0201<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_ksc5601<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_latin2<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_one_one<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_roman<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_sjis<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_sjis_0208<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_turkish<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_utf16<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_utf8<wchar_t> >());
    BOOST_CHECK(codecvt_test< codecvt_utf8_utf16<wchar_t> >());
#endif
}

/* Defer pending further testing
void close_test()  
{
    typedef utf8_codecvt_facet<wchar_t, char> codecvt_type;

    // Test code converter based on a source
    {
        operation_sequence  seq;
        io::wchain<input>   ch;
        ch.push(
            code_converter<closable_device<input>, codecvt_type>(
                seq.new_operation(1)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test code converter based on a sink
    {
        operation_sequence   seq;
        io::wchain<output>    ch;
        ch.push(
            code_converter<closable_device<output>, codecvt_type>(
                seq.new_operation(1)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test code converter based on a bidirectional device
    {
        operation_sequence         seq;
        io::wchain<bidirectional>  ch;
        ch.push(
            code_converter<closable_device<bidirectional>, codecvt_type>(
                seq.new_operation(1), 
                seq.new_operation(2)
            )
        );
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}*/

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("code_converter test");
    test->add(BOOST_TEST_CASE(&code_converter_test));
    //test->add(BOOST_TEST_CASE(&close_test));
    return test;
}
