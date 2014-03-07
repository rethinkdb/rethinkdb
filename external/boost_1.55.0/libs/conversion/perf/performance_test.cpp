//  (C) Copyright Antony Polukhin 2012.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//
// Testing lexical_cast<> performance
//

#define BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_CHRONO_HEADER_ONLY

#include <boost/lexical_cast.hpp>

#include <boost/chrono.hpp>
#include <fstream>
#include <cstring>
#include <boost/container/string.hpp>

// File to output data
std::fstream fout;

namespace boost {
inline std::istream& operator>> (std::istream& in, boost::array<char,50>& res) {
    in >> res.begin();
    return in;
}
}

template <class OutT, class InT>
static inline void test_lexical(const InT& in_val) {
    OutT out_val = boost::lexical_cast<OutT>(in_val);
    (void)out_val;
}

template <class OutT, class InT>
static inline void test_ss_constr(const InT& in_val) {
    OutT out_val;
    std::stringstream ss;
    ss << in_val;
    if (ss.fail()) throw std::logic_error("descr");
    ss >> out_val;
    if (ss.fail()) throw std::logic_error("descr");
}

template <class OutT, class CharT, std::size_t N>
static inline void test_ss_constr(const boost::array<CharT, N>& in_val) {
    OutT out_val;
    std::stringstream ss;
    ss << in_val.begin();
    if (ss.fail()) throw std::logic_error("descr");
    ss >> out_val;
    if (ss.fail()) throw std::logic_error("descr");
}

template <class OutT, class StringStreamT, class CharT, std::size_t N>
static inline void test_ss_noconstr(StringStreamT& ss, const boost::array<CharT, N>& in_val) {
    OutT out_val;
    ss << in_val.begin(); // ss is an instance of std::stringstream
    if (ss.fail()) throw std::logic_error("descr");
    ss >> out_val;
    if (ss.fail()) throw std::logic_error("descr");
    /* reseting std::stringstream to use it again */
    ss.str(std::string());
    ss.clear();
}

template <class OutT, class StringStreamT, class InT>
static inline void test_ss_noconstr(StringStreamT& ss, const InT& in_val) {
    OutT out_val;
    ss << in_val; // ss is an instance of std::stringstream
    if (ss.fail()) throw std::logic_error("descr");
    ss >> out_val;
    if (ss.fail()) throw std::logic_error("descr");
    /* reseting std::stringstream to use it again */
    ss.str(std::string());
    ss.clear();
}

struct structure_sprintf {
    template <class OutT, class BufferT, class InT>
    static inline void test(BufferT* buffer, const InT& in_val, const char* const conv) {
        sprintf(buffer, conv, in_val);
        OutT out_val(buffer);
    }

    template <class OutT, class BufferT>
    static inline void test(BufferT* buffer, const std::string& in_val, const char* const conv) {
        sprintf(buffer, conv, in_val.c_str());
        OutT out_val(buffer);
    }
};

struct structure_sscanf {
    template <class OutT, class BufferT, class CharT, std::size_t N>
    static inline void test(BufferT* /*buffer*/, const boost::array<CharT, N>& in_val, const char* const conv) {
        OutT out_val;
        sscanf(in_val.cbegin(), conv, &out_val);
    }

    template <class OutT, class BufferT, class InT>
    static inline void test(BufferT* /*buffer*/, const InT& in_val, const char* const conv) {
        OutT out_val;
        sscanf(reinterpret_cast<const char*>(in_val), conv, &out_val);
    }

    template <class OutT, class BufferT>
    static inline void test(BufferT* /*buffer*/, const std::string& in_val, const char* const conv) {
        OutT out_val;
        sscanf(in_val.c_str(), conv, &out_val);
    }

    template <class OutT, class BufferT>
    static inline void test(BufferT* /*buffer*/, const boost::iterator_range<const char*>& in_val, const char* const conv) {
        OutT out_val;
        sscanf(in_val.begin(), conv, &out_val);
    }
};

struct structure_fake {
    template <class OutT, class BufferT, class InT>
    static inline void test(BufferT* /*buffer*/, const InT& /*in_val*/, const char* const /*conv*/) {}
};

static const int fake_test_value = 9999;

template <class T>
static inline void min_fancy_output(T v1, T v2, T v3, T v4) {
    const char beg_mark[] = "!!! *";
    const char end_mark[] = "* !!!";
    const char no_mark[] = "";

    unsigned int res = 4;
    if (v1 < v2 && v1 < v3 && v1 < v4) res = 1;
    if (v2 < v1 && v2 < v3 && v2 < v4) res = 2;
    if (v3 < v1 && v3 < v2 && v3 < v4) res = 3;

    fout << "[ "
         << (res == 1 ? beg_mark : no_mark)
         ;

    if (v1) fout << v1;
    else fout << "<1";

    fout << (res == 1 ? end_mark : no_mark)
         << " ][ "
         << (res == 2 ? beg_mark : no_mark)
         ;

       if (v2) fout << v2;
       else fout << "<1";

       fout << (res == 2 ? end_mark : no_mark)
         << " ][ "
         << (res == 3 ? beg_mark : no_mark)
         ;

       if (v3) fout << v3;
       else fout << "<1";

       fout << (res == 3 ? end_mark : no_mark)
         << " ][ "
         << (res == 4 ? beg_mark : no_mark)
         ;

       if (!v4) fout << "<1";
       else if (v4 == fake_test_value) fout << "---";
       else fout << v4;

       fout
         << (res == 4 ? end_mark : no_mark)
         << " ]";
}

template <unsigned int IetartionsCountV, class ToT, class SprintfT, class FromT>
static inline void perf_test_impl(const FromT& in_val, const char* const conv) {

    typedef boost::chrono::steady_clock test_clock;
    test_clock::time_point start;
    typedef boost::chrono::milliseconds duration_t;
    duration_t lexical_cast_time, ss_constr_time, ss_noconstr_time, printf_time;

    start = test_clock::now();
    for (unsigned int i = 0; i < IetartionsCountV; ++i) {
        test_lexical<ToT>(in_val);
        test_lexical<ToT>(in_val);
        test_lexical<ToT>(in_val);
        test_lexical<ToT>(in_val);
    }
    lexical_cast_time = boost::chrono::duration_cast<duration_t>(test_clock::now() - start);


    start = test_clock::now();
    for (unsigned int i = 0; i < IetartionsCountV; ++i) {
        test_ss_constr<ToT>(in_val);
        test_ss_constr<ToT>(in_val);
        test_ss_constr<ToT>(in_val);
        test_ss_constr<ToT>(in_val);
    }
    ss_constr_time = boost::chrono::duration_cast<duration_t>(test_clock::now() - start);

    std::stringstream ss;
    start = test_clock::now();
    for (unsigned int i = 0; i < IetartionsCountV; ++i) {
        test_ss_noconstr<ToT>(ss, in_val);
        test_ss_noconstr<ToT>(ss, in_val);
        test_ss_noconstr<ToT>(ss, in_val);
        test_ss_noconstr<ToT>(ss, in_val);
    }
    ss_noconstr_time = boost::chrono::duration_cast<duration_t>(test_clock::now() - start);


    char buffer[128];
    start = test_clock::now();
    for (unsigned int i = 0; i < IetartionsCountV; ++i) {
        SprintfT::template test<ToT>(buffer, in_val, conv);
        SprintfT::template test<ToT>(buffer, in_val, conv);
        SprintfT::template test<ToT>(buffer, in_val, conv);
        SprintfT::template test<ToT>(buffer, in_val, conv);
    }
    printf_time = boost::chrono::duration_cast<duration_t>(test_clock::now() - start);

    min_fancy_output(
                lexical_cast_time.count(),
                ss_constr_time.count(),
                ss_noconstr_time.count(),
                boost::is_same<SprintfT, structure_fake>::value ? fake_test_value : printf_time.count()
    );
}

template <class ToT, class SprintfT, class FromT>
static inline void perf_test(const std::string& test_name, const FromT& in_val, const char* const conv) {
    const unsigned int ITERATIONSCOUNT = 100000;
    fout << "  [[ " << test_name << " ]";

    perf_test_impl<ITERATIONSCOUNT/4, ToT, SprintfT>(in_val, conv);

    fout << "]\n";
}


template <class ConverterT>
void string_like_test_set(const std::string& from) {
    typedef structure_sscanf ssc_t;
    ConverterT conv;

    perf_test<char, ssc_t>(from + "->char",               conv("c"), "%c");
    perf_test<signed char, ssc_t>(from + "->signed char", conv("c"), "%hhd");
    perf_test<unsigned char, ssc_t>(from + "->unsigned char", conv("c"), "%hhu");

    perf_test<int, ssc_t>(from + "->int",             conv("100"), "%d");
    perf_test<short, ssc_t>(from + "->short",         conv("100"), "%hd");
    perf_test<long int, ssc_t>(from + "->long int",   conv("100"), "%ld");
    perf_test<boost::long_long_type, ssc_t>(from + "->long long", conv("100"), "%lld");

    perf_test<unsigned int, ssc_t>(from + "->unsigned int",             conv("100"), "%u");
    perf_test<unsigned short, ssc_t>(from + "->unsigned short",         conv("100"), "%hu");
    perf_test<unsigned long int, ssc_t>(from + "->unsigned long int",   conv("100"), "%lu");
    perf_test<boost::ulong_long_type, ssc_t>(from + "->unsigned long long", conv("100"), "%llu");

    // perf_test<bool, ssc_t>(from + "->bool", conv("1"), "%");

    perf_test<float, ssc_t>(from + "->float",             conv("1.123"), "%f");
    perf_test<double, ssc_t>(from + "->double",           conv("1.123"), "%lf");
    perf_test<long double, ssc_t>(from + "->long double", conv("1.123"), "%Lf");
    perf_test<boost::array<char, 50>, ssc_t>(from + "->array<char, 50>", conv("1.123"), "%s");

    perf_test<std::string, structure_fake>(from + "->string", conv("string"), "%Lf");
    perf_test<boost::container::string, structure_fake>(from + "->container::string"
                                                        , conv("string"), "%Lf");

}

struct to_string_conv {
    std::string operator()(const char* const c) const {
        return c;
    }
};

struct to_char_conv {
    const char*  operator()(const char* const c) const {
        return c;
    }
};

struct to_uchar_conv {
    const unsigned char*  operator()(const char* const c) const {
        return reinterpret_cast<const unsigned char*>(c);
    }
};


struct to_schar_conv {
    const signed char*  operator()(const char* const c) const {
        return reinterpret_cast<const signed char*>(c);
    }
};

struct to_iterator_range {
    boost::iterator_range<const char*>  operator()(const char* const c) const {
        return boost::make_iterator_range(c, c + std::strlen(c));
    }
};

struct to_array_50 {
    boost::array<char, 50> operator()(const char* const c) const {
        boost::array<char, 50> ret;
        std::strcpy(ret.begin(), c);
        return ret;
    }
};

int main(int argc, char** argv) {
    BOOST_ASSERT(argc >= 2);
    std::string output_path(argv[1]);
    output_path += "/results.txt";
    fout.open(output_path.c_str(), std::fstream::in | std::fstream::out | std::fstream::app);
    BOOST_ASSERT(fout);

    fout << "[section " << BOOST_COMPILER << "]\n"
        << "[table:id Performance Table ( "<< BOOST_COMPILER << ")\n"
        << "[[From->To] [lexical_cast] [std::stringstream with construction] "
        << "[std::stringstream without construction][scanf/printf]]\n";


    // From std::string to ...
    string_like_test_set<to_string_conv>("string");

    // From ... to std::string
    perf_test<std::string, structure_sprintf>("string->char",                'c', "%c");
    perf_test<std::string, structure_sprintf>("string->signed char",     static_cast<signed char>('c'), "%hhd");
    perf_test<std::string, structure_sprintf>("string->unsigned char", static_cast<unsigned char>('c'), "%hhu");

    perf_test<std::string, structure_sprintf>("int->string",                 100, "%d");
    perf_test<std::string, structure_sprintf>("short->string",               static_cast<short>(100), "%hd");
    perf_test<std::string, structure_sprintf>("long int->string",            100l, "%ld");
    perf_test<std::string, structure_sprintf>("long long->string",           100ll, "%lld");

    perf_test<std::string, structure_sprintf>("unsigned int->string",        static_cast<unsigned short>(100u), "%u");
    perf_test<std::string, structure_sprintf>("unsigned short->string",      100u, "%hu");
    perf_test<std::string, structure_sprintf>("unsigned long int->string",   100ul, "%lu");
    perf_test<std::string, structure_sprintf>("unsigned long long->string",  static_cast<boost::ulong_long_type>(100), "%llu");

    // perf_test<bool, structure_sscanf>("bool->string", std::string("1"), "%");

    perf_test<std::string, structure_sprintf>("float->string",       1.123f, "%f");
    perf_test<std::string, structure_sprintf>("double->string",      1.123, "%lf");
    perf_test<std::string, structure_sprintf>("long double->string", 1.123L, "%Lf");


    string_like_test_set<to_char_conv>("char*");
    string_like_test_set<to_uchar_conv>("unsigned char*");
    string_like_test_set<to_schar_conv>("signed char*");
    string_like_test_set<to_iterator_range>("iterator_range<char*>");
    string_like_test_set<to_array_50>("array<char, 50>");

    perf_test<int, structure_fake>("int->int", 100, "");
    perf_test<double, structure_fake>("float->double", 100.0f, "");
    perf_test<signed char, structure_fake>("char->signed char", 'c', "");


    fout << "]\n"
        << "[endsect]\n\n";
    return 0;
}


