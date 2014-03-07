//  (C) Copyright Andy Tompkins 2007. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Purpose to test serializing uuids with narrow archives

#include <boost/detail/lightweight_test.hpp>
#include <sstream>
#include <iostream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_serialize.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

template <class OArchiveType, class IArchiveType, class OStringStreamType, class IStringStreamType>
void test_archive()
{
    using namespace std;
    using namespace boost::uuids;
    
    OStringStreamType o_stream;
    
    uuid u1 = {{0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef}};

    uuid u2;
    
    // save
    {
        OArchiveType oa(o_stream);
        
        oa << BOOST_SERIALIZATION_NVP(u1);
    }
    
    //cout << "stream:" << o_stream.str() << "\n\n";
    
    // load
    {
        IStringStreamType i_stream(o_stream.str());
        IArchiveType ia(i_stream);
        
        ia >> BOOST_SERIALIZATION_NVP(u2);
    }
    
    BOOST_TEST_EQ(u1, u2);
}

int main( int /* argc */, char* /* argv */[] )
{
    using namespace std;
    using namespace boost::archive;
    
    test_archive<text_oarchive, text_iarchive, ostringstream, istringstream>();
    test_archive<xml_oarchive, xml_iarchive, ostringstream, istringstream>();
    test_archive<binary_oarchive, binary_iarchive, ostringstream, istringstream>();

    return boost::report_errors();
}
