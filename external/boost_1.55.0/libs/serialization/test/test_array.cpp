/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_array.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <cstddef>
#include <fstream>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/array.hpp>

#include "A.hpp"
#include "A.ipp"

struct array_equal_to //: public std::binary_function<T, T, bool>
{
template<class T, class U>
    bool operator()(const T & _Left, const U & _Right) const
    {
        // consider alignment
        int count_left = sizeof(_Left) /    (
            static_cast<const char *>(static_cast<const void *>(&_Left[1])) 
            - static_cast<const char *>(static_cast<const void *>(&_Left[0]))
        );
        int count_right = sizeof(_Right) /  (
            static_cast<const char *>(static_cast<const void *>(&_Right[1])) 
            - static_cast<const char *>(static_cast<const void *>(&_Right[0]))
        );
        if(count_right != count_left)
            return false;
        while(count_left-- > 0){
            if(_Left[count_left] == _Right[count_left])
                continue;
            return false;
        }
        return true;
    }
};

template <class T>
int test_array(T)
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    // test array of objects
    const T a_array[10]={T(),T(),T(),T(),T(),T(),T(),T(),T(),T()};
    const T b_array[2][3]={{T(),T(),T()},{T(),T(),T()}};
    const boost::array<T,10> c_array = boost::array<T,10>();
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("a_array", a_array);
        oa << boost::serialization::make_nvp("b_array", b_array);
        oa << boost::serialization::make_nvp("c_array", c_array);
    }
    {
        T a_array1[10];
        T b_array1[2][3];
        boost::array<T,10> c_array1;
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("a_array", a_array1);
        ia >> boost::serialization::make_nvp("b_array", b_array1);
        ia >> boost::serialization::make_nvp("c_array", c_array1);

        array_equal_to/*<A[10]>*/ Compare;
        BOOST_CHECK(Compare(a_array, a_array1));
        BOOST_CHECK(Compare(b_array[0], b_array1[0]));
        BOOST_CHECK(Compare(b_array[1], b_array1[1]));
        BOOST_CHECK(Compare(c_array, c_array1));
    }
    {
        T a_array1[9];
        test_istream is(testfile, TEST_STREAM_FLAGS);
        BOOST_TRY {
            test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
            bool exception_invoked = false;
            BOOST_TRY {
                ia >> boost::serialization::make_nvp("a_array", a_array1);
            }
            BOOST_CATCH (boost::archive::archive_exception ae){
                BOOST_CHECK(
                    boost::archive::archive_exception::array_size_too_short
                    == ae.code
                );
                exception_invoked = true;
            }
            BOOST_CATCH_END
            BOOST_CHECK(exception_invoked);
        }
        BOOST_CATCH (boost::archive::archive_exception ae){}
        BOOST_CATCH_END
    }
    std::remove(testfile);
    return EXIT_SUCCESS;
}


int test_main( int /* argc */, char* /* argv */[] )
{
   int res = test_array(A());
    // test an int array for which optimized versions should be available
   if (res == EXIT_SUCCESS)
     res = test_array(0);  
   return res;
}

// EOF
