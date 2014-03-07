//  (C) Copyright 2009 Robert Ramey
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

// note: this is a compile only test.
#include <sstream>
#include <boost/config.hpp> // BOOST_STATIC_CONST

#include <boost/serialization/static_warning.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/nvp.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

// track_selectivly with class information in the archive 
// is unsafe when used with a pointer and should trigger a warning
struct check1 {
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    check1(){}
};

BOOST_CLASS_IMPLEMENTATION(check1, boost::serialization::object_serializable)
BOOST_CLASS_TRACKING(check1, boost::serialization::track_selectively)

// the combination of versioning + no class information
// should trigger a warning
struct check2 {
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    check2(){}
};

BOOST_CLASS_IMPLEMENTATION(check2, boost::serialization::object_serializable)
BOOST_CLASS_VERSION(check2, 1)
// use track always to turn off warning tested above
BOOST_CLASS_TRACKING(check2, boost::serialization::track_always)

// serializing a type marked as "track_never" through a pointer
// is likely an error
struct check3 {
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    check3(){}
};

BOOST_CLASS_TRACKING(check3, boost::serialization::track_never)

template<class T>
int f(){
    BOOST_STATIC_WARNING(T::value);
    BOOST_STATIC_ASSERT(T::value);
    return 0;
}

/////////////////////////////////////////////////////////////////////////
// compilation of this program should show a total of 10 warning messages
int main(int /* argc */, char * /* argv */[]){
    std::stringstream s;
    {
        boost::archive::text_oarchive oa(s);

        check1 const c1_out;
        oa << c1_out;

        check1 c1_non_const_out;
        oa << c1_non_const_out; // warn check_object_tracking

        check1 * const c1_ptr_out = 0;
        oa << c1_ptr_out; // warn check_pointer_level

        check2 const * c2_ptr_out;
        oa << c2_ptr_out; // error check_object_versioning

        check3 * const c3_ptr_out = 0;
        oa << c3_ptr_out; // warning check_pointer_tracking

        check2 const c2_out;
        oa << c2_out; // error check_object_versioning
    }
    {
        boost::archive::text_iarchive ia(s);

        check1 const c1_in;
        ia >> c1_in; // check_const_loading

        check1 * c1_ptr_in = 0;
        ia >> c1_ptr_in; // warn check_pointer_level

        check2 * c2_ptr_in;
        ia >> c2_ptr_in; // error check_object_versioning

        check3 * c3_ptr_in = 0;
        ia >> c3_ptr_in; // warning check_pointer_tracking

        check2 c2_in;
        ia >> c2_in; // error check_object_versioning
    }
    {
        boost::archive::text_oarchive oa(s);

        check1 const c1_out;
        oa << BOOST_SERIALIZATION_NVP(c1_out);

        check1 c1_non_const_out;
        oa << BOOST_SERIALIZATION_NVP(c1_non_const_out); // warn check_object_tracking

        check1 * const c1_ptr_out = 0;
        oa << BOOST_SERIALIZATION_NVP(c1_ptr_out); // warn check_pointer_level

        check2 const * c2_ptr_out;
        oa << BOOST_SERIALIZATION_NVP(c2_ptr_out); // error check_object_versioning

        check3 * const c3_ptr_out = 0;
        oa << BOOST_SERIALIZATION_NVP(c3_ptr_out); // warning check_pointer_tracking

        check2 const c2_out;
        oa << BOOST_SERIALIZATION_NVP(c2_out); // error check_object_versioning
    }
    {
        boost::archive::text_iarchive ia(s);

        check1 const c1_in;
        ia >> BOOST_SERIALIZATION_NVP(c1_in); // check_const_loading

        check1 * c1_ptr_in = 0;
        ia >> BOOST_SERIALIZATION_NVP(c1_ptr_in); // warn check_pointer_level

        check2 * c2_ptr_in;
        ia >> BOOST_SERIALIZATION_NVP(c2_ptr_in); // error check_object_versioning

        check3 * c3_ptr_in = 0;
        ia >> BOOST_SERIALIZATION_NVP(c3_ptr_in); // warning check_pointer_tracking

        check2 c2_in;
        ia >> BOOST_SERIALIZATION_NVP(c2_in); // error check_object_versioning
    }
    return 0;
}
