//  (C) Copyright Andy Tompkins 2009. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  libs/uuid/test/test_io.cpp  -------------------------------//

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/lexical_cast.hpp>
#include <string>
#include <sstream>
#include <iomanip>

#ifndef BOOST_NO_STD_WSTRING
namespace std {
template <typename Elem, typename Traits>
std::basic_ostream<Elem, Traits>& operator<<(std::basic_ostream<Elem, Traits>& os, std::wstring const& s) {
    // convert to string
    std::string temp(s.begin(), s.end());
    os << temp;
    return os;
}
} // namespace std
#endif

int main(int, char*[])
{
    using namespace boost::uuids;

    const uuid u1 = {{0}};
    const uuid u2 = {{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}};
    const uuid u3 = {{0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef,0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef}};

    { // test insert/extract operators
        std::stringstream ss1;
        ss1 << u1;
        BOOST_TEST_EQ(ss1.str(), "00000000-0000-0000-0000-000000000000");

        std::stringstream ss2;
        ss2 << u2;
        BOOST_TEST_EQ(ss2.str(), "00010203-0405-0607-0809-0a0b0c0d0e0f");

        std::stringstream ss3;
        ss3 << u3;
        BOOST_TEST_EQ(ss3.str(), "12345678-90ab-cdef-1234-567890abcdef");
        
        std::stringstream ss4;
        ss4 << std::uppercase << u3;
        BOOST_TEST_EQ(ss4.str(), "12345678-90AB-CDEF-1234-567890ABCDEF");
        
        std::stringstream ss5;
        ss5 << 'a' << std::right << std::setfill('*') << std::setw(40) << u1 << 'a';
        BOOST_TEST_EQ(ss5.str(), "a****00000000-0000-0000-0000-000000000000a");
        
        std::stringstream ss6;
        ss6 << std::left << std::setfill('*') << std::setw(45) << u1;
        BOOST_TEST_EQ(ss6.str(), "00000000-0000-0000-0000-000000000000*********");
    }

 #ifndef BOOST_NO_STD_WSTRING
    { // test insert/extract operators
        std::wstringstream ss1;
        ss1 << u1;
        BOOST_TEST_EQ(ss1.str(), L"00000000-0000-0000-0000-000000000000");

        std::wstringstream ss2;
        ss2 << u2;
        BOOST_TEST_EQ(ss2.str(), L"00010203-0405-0607-0809-0a0b0c0d0e0f");

        std::wstringstream ss3;
        ss3 << u3;
        BOOST_TEST_EQ(ss3.str(), L"12345678-90ab-cdef-1234-567890abcdef");
        
        std::wstringstream ss4;
        ss4 << std::uppercase << u3;
        BOOST_TEST_EQ(ss4.str(), L"12345678-90AB-CDEF-1234-567890ABCDEF");
        
        std::wstringstream ss5;
        ss5 << L'a' << std::right << std::setfill(L'*') << std::setw(40) << u1 << L'a';
        BOOST_TEST_EQ(ss5.str(), L"a****00000000-0000-0000-0000-000000000000a");
        
        std::wstringstream ss6;
        ss6 << std::left << std::setfill(L'*') << std::setw(45) << u1;
        BOOST_TEST_EQ(ss6.str(), L"00000000-0000-0000-0000-000000000000*********");
    }
#endif

    {
        uuid u;

        std::stringstream ss;
        ss << "00000000-0000-0000-0000-000000000000";
        ss >> u;
        BOOST_TEST_EQ(u, u1);

        ss << "12345678-90ab-cdef-1234-567890abcdef";
        ss >> u;
        BOOST_TEST_EQ(u, u3);
    }

 #ifndef BOOST_NO_STD_WSTRING
   {
        uuid u;

        std::wstringstream ss;
        ss << L"00000000-0000-0000-0000-000000000000";
        ss >> u;
        BOOST_TEST_EQ(u, u1);

        ss << L"12345678-90ab-cdef-1234-567890abcdef";
        ss >> u;
        BOOST_TEST_EQ(u, u3);
    }
#endif

    { // test with lexical_cast
        BOOST_TEST_EQ(boost::lexical_cast<std::string>(u1), std::string("00000000-0000-0000-0000-000000000000"));
        BOOST_TEST_EQ(boost::lexical_cast<uuid>("00000000-0000-0000-0000-000000000000"), u1);

        BOOST_TEST_EQ(boost::lexical_cast<std::string>(u3), std::string("12345678-90ab-cdef-1234-567890abcdef"));
        BOOST_TEST_EQ(boost::lexical_cast<uuid>("12345678-90ab-cdef-1234-567890abcdef"), u3);
    }

#ifndef BOOST_NO_STD_WSTRING
    { // test with lexical_cast
        BOOST_TEST_EQ(boost::lexical_cast<std::wstring>(u1), std::wstring(L"00000000-0000-0000-0000-000000000000"));
        BOOST_TEST_EQ(boost::lexical_cast<uuid>(L"00000000-0000-0000-0000-000000000000"), u1);

        BOOST_TEST_EQ(boost::lexical_cast<std::wstring>(u3), std::wstring(L"12345678-90ab-cdef-1234-567890abcdef"));
        BOOST_TEST_EQ(boost::lexical_cast<uuid>(L"12345678-90ab-cdef-1234-567890abcdef"), u3);
    }
#endif
    
    { // test to_string
        BOOST_TEST_EQ(to_string(u1), std::string("00000000-0000-0000-0000-000000000000"));
        BOOST_TEST_EQ(to_string(u3), std::string("12345678-90ab-cdef-1234-567890abcdef"));
    }
    
#ifndef BOOST_NO_STD_WSTRING
    { // test to_wstring
        BOOST_TEST_EQ(to_wstring(u1), std::wstring(L"00000000-0000-0000-0000-000000000000"));
        BOOST_TEST_EQ(to_wstring(u3), std::wstring(L"12345678-90ab-cdef-1234-567890abcdef"));
    }
#endif

    return boost::report_errors();
}
