/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// demo_auto_ptr.cpp

// (C) Copyright 2002-4 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <list>
#include <memory>
#include <fstream>
#include <string>

#include <cstdio> // remove, std::autoptr inteface wrong in dinkumware
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include <boost/archive/tmpdir.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/split_free.hpp>

namespace boost { 
namespace serialization {

/////////////////////////////////////////////////////////////
// implement serialization for auto_ptr< T >
// note: this must be added to the boost namespace in order to
// be called by the library
template<class Archive, class T>
inline void save(
    Archive & ar,
    const std::auto_ptr< T > &t,
    const unsigned int file_version
){
    // only the raw pointer has to be saved
    // the ref count is rebuilt automatically on load
    const T * const tx = t.get();
    ar << tx;
}

template<class Archive, class T>
inline void load(
    Archive & ar,
    std::auto_ptr< T > &t,
    const unsigned int file_version
){
    T *pTarget;
    ar >> pTarget;
    // note that the reset automagically maintains the reference count
    #if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
        t.release();
        t = std::auto_ptr< T >(pTarget);
    #else
        t.reset(pTarget);
    #endif
}

// split non-intrusive serialization function member into separate
// non intrusive save/load member functions
template<class Archive, class T>
inline void serialize(
    Archive & ar,
    std::auto_ptr< T > &t,
    const unsigned int file_version
){
    boost::serialization::split_free(ar, t, file_version);
}

} // namespace serialization
} // namespace boost

/////////////////////////////////////////////////////////////
// test auto_ptr serialization
class A
{
private:
    friend class boost::serialization::access;
    int x;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & x;
    }
public:
    A(){}    // default constructor
    ~A(){}   // default destructor
};

void save(const std::auto_ptr<A> & spa, const char *filename)
{
    std::ofstream ofs(filename);
    boost::archive::text_oarchive oa(ofs);
    oa << spa;
}

void load(std::auto_ptr<A> & spa, const char *filename)
{
    // open the archive
    std::ifstream ifs(filename);
    boost::archive::text_iarchive ia(ifs);

    // restore the schedule from the archive
    ia >> spa;
}

int main(int argc, char *argv[])
{
    std::string filename = boost::archive::tmpdir();
    filename += "/testfile";

    // create  a new auto pointer to ta new object of type A
    std::auto_ptr<A> spa(new A);
    // serialize it
    save(spa, filename.c_str());
    // reset the auto pointer to NULL
    // thereby destroying the object of type A
    // note that the reset automagically maintains the reference count
    #if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
        spa.release();
    #else
        spa.reset();
    #endif
    // restore state to one equivalent to the original
    // creating a new type A object
    load(spa, filename.c_str());
    // obj of type A gets destroyed
    // as auto_ptr goes out of scope
    std::remove(filename.c_str());
    return 0;
}
