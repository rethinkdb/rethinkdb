/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_unregistered.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <fstream>

#include <cstddef> // NULL
#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"

#include <boost/archive/archive_exception.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/type_info_implementation.hpp>
#include <boost/detail/no_exceptions_support.hpp>

class polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & /* ar */, const unsigned int /* file_version */){
    }
public:
    virtual ~polymorphic_base(){};
};

class polymorphic_derived1 : public polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(polymorphic_base);
    }
};

class polymorphic_derived2 : public polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(polymorphic_base);
    }
};

// save unregistered polymorphic classes
void save_unregistered1(const char *testfile)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);

    polymorphic_base *rb1 =  new polymorphic_derived1;
    
    // registration IS necessary when serializing a polymorphic class
    // through pointer to the base class
    bool except = false;
    BOOST_TRY {
        oa << BOOST_SERIALIZATION_NVP(rb1);
    }
    BOOST_CATCH(boost::archive::archive_exception aex){
        except = true;
    }
    BOOST_CATCH_END
    BOOST_CHECK_MESSAGE(except, "lack of registration not detected !");

    delete rb1;
}

// note: the corresponding save function above will not result in
// valid archive - hence, the following code which attempts to load
// and archive will fail.  Leave this as a reminder not to do this
#if 0
// load unregistered polymorphic classes
void load_unregistered1(const char *testfile)
{
    std::ifstream is(testfile);
    boost::archive::iarchive ia(is);

    polymorphic_base *rb1(NULL);

    // registration IS necessary when serializing a polymorphic class
    // through pointer to the base class
    bool except = false;
    BOOST_TRY {
        ia >> BOOST_SERIALIZATION_NVP(rb1);
    }
    BOOST_CATCH(boost::archive::archive_exception aex){
        except = true;
        BOOST_CHECK_MESSAGE(
            NULL == rb1, 
            "failed load resulted in a non-null pointer"
        );
    }
    BOOST_CATCH_END
    BOOST_CHECK_MESSAGE(except, "lack of registration not detected !");

    delete rb1;
}
#endif

// save unregistered polymorphic classes
void save_unregistered2(const char *testfile)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);

    polymorphic_derived1 *rd1 = new polymorphic_derived1;
    
    // registration is NOT necessary when serializing a polymorphic class
    // through pointer to a derived class
    bool except = false;
    BOOST_TRY {
        oa << BOOST_SERIALIZATION_NVP(rd1);
    }
    BOOST_CATCH(boost::archive::archive_exception aex){
        except = true;
    }
    BOOST_CATCH_END
    BOOST_CHECK_MESSAGE(! except, "registration not detected !");

    delete rd1;
}

// note: the corresponding save function above will not result in
// valid archive - hence, the following code which attempts to load
// and archive will fail.  Leave this as a reminder not to do this
// load unregistered polymorphic classes
void load_unregistered2(const char *testfile)
{
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);

    polymorphic_derived1 *rd1 = NULL;
    
    // registration is NOT necessary when serializing a polymorphic class
    // or through pointer to a derived class
    bool except = false;
    BOOST_TRY {
        ia >> BOOST_SERIALIZATION_NVP(rd1);
    }
    BOOST_CATCH(boost::archive::archive_exception aex){
        except = true;
        BOOST_CHECK_MESSAGE(
            NULL == rd1, 
            "failed load resulted in a non-null pointer"
        );
    }
    BOOST_CATCH_END
    BOOST_CHECK_MESSAGE(! except, "registration not detected !");

    delete rd1;
}

// save registered polymorphic class
void save_registered(const char *testfile)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);

    polymorphic_base *rb1 = new polymorphic_derived1;
    polymorphic_base *rb2 = new polymorphic_derived2;

    // registration (forward declaration) will permit correct serialization
    // through a pointer to a base class
    oa.register_type(static_cast<polymorphic_derived1 *>(NULL));
    oa.register_type(static_cast<polymorphic_derived2 *>(NULL));
    oa << BOOST_SERIALIZATION_NVP(rb1); 
    oa << BOOST_SERIALIZATION_NVP(rb2);

    delete rb1;
    delete rb2;
}

// load registered polymorphic class
void load_registered(const char *testfile)
{
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);

    polymorphic_base *rb1 = NULL;
    polymorphic_base *rb2 = NULL;

    // registration (forward declaration) will permit correct serialization
    // through a pointer to a base class
    ia.register_type(static_cast<polymorphic_derived1 *>(NULL));
    ia.register_type(static_cast<polymorphic_derived2 *>(NULL));

    ia >> BOOST_SERIALIZATION_NVP(rb1);

    BOOST_CHECK_MESSAGE(NULL != rb1, "Load resulted in NULL pointer");
    BOOST_CHECK_MESSAGE(
        boost::serialization::type_info_implementation<
            polymorphic_derived1
        >::type::get_const_instance()
        == 
        * boost::serialization::type_info_implementation<
            polymorphic_base
        >::type::get_const_instance().get_derived_extended_type_info(*rb1),
        "restored pointer b1 not of correct type"
    );

    ia >> BOOST_SERIALIZATION_NVP(rb2);
    BOOST_CHECK_MESSAGE(NULL != rb2, "Load resulted in NULL pointer");
    BOOST_CHECK_MESSAGE(
        boost::serialization::type_info_implementation<
            polymorphic_derived2
        >::type::get_const_instance()
        == 
        * boost::serialization::type_info_implementation<
            polymorphic_base
        >::type::get_const_instance().get_derived_extended_type_info(*rb2),
        "restored pointer b2 not of correct type"
    );

    delete rb1;
    delete rb2;
}

int
test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);
    save_unregistered1(testfile);
//  load_unregistered1(testfile);
    save_unregistered2(testfile);
    load_unregistered2(testfile);
    save_registered(testfile);
    load_registered(testfile);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
