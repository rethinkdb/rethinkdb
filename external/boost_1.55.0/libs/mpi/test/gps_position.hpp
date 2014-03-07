#ifndef GPS_POSITION_HPP
#define GPS_POSITION_HPP

// Copyright Matthias Troyer
// 2005. Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/mpi/datatype_fwd.hpp>
#include <boost/mpl/and.hpp>
#include <boost/serialization/export.hpp>
#include <boost/shared_ptr.hpp>

class gps_position
{
private:
    friend class boost::serialization::access;
    // When the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<.  Likewise, when the class Archive
    // is a type of input archive the & operator is defined similar to >>.
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & degrees & minutes & seconds;
    }
    int degrees;
    int minutes;
    float seconds;
public:
    gps_position(){};
    gps_position(int d, int m, float s) :
        degrees(d), minutes(m), seconds(s)
    {}

  friend bool operator==(const gps_position& x, const gps_position& y)
  {
    return (x.degrees == y.degrees
            && x.minutes == y.minutes
            && x.seconds == y.seconds);
  }

  inline friend bool operator!=(const gps_position& x, const gps_position& y)
  {
    return !(x == y);
  }
};


namespace boost { namespace mpi {

  template <>
  struct is_mpi_datatype<gps_position>
   : public mpl::and_
       <
                 is_mpi_datatype<int>,
                 is_mpi_datatype<float>
           >
  {};

} }
#endif
