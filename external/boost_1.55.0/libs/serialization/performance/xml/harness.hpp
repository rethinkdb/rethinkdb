/* /libs/serialization/xml_performance/harness.hpp *****************************

(C) Copyright 2010 Bryce Lelbach

Use, modification and distribution is subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
http://www.boost.org/LICENSE_1_0.txt)

*******************************************************************************/

#if !defined(BOOST_SERIALIZATION_XML_PERFORMANCE_HARNESS_HPP)
#define BOOST_SERIALIZATION_XML_PERFORMANCE_HARNESS_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
  # pragma once
#endif

#include <cassert>
#include <cstdio> 

#include <string>
#include <fstream>
#include <iostream>
#include <utility>

#include <boost/config.hpp>

#if defined(BOOST_NO_STDC_NAMESPACE)
  namespace std { 
    using ::remove;
  }
#endif 

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

#include <boost/functional/hash.hpp>

#include <boost/utility/enable_if.hpp>

#include <boost/type_traits/is_integral.hpp>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <boost/archive/archive_exception.hpp>

#include "high_resolution_timer.hpp" // from /libs/spirit/optimization

#include "node.hpp" // includes macro.hpp

namespace boost {
namespace archive {
namespace xml {

template<typename T> T random (void);

template<typename T> T
random (void) {
  using namespace boost::uuids;

  hash<uuid> hash;
  basic_random_generator<mt19937> gen;

  return hash(gen());
}

template<> std::string
random<std::string> (void) {
  using namespace boost::uuids;

  basic_random_generator<mt19937> gen;
  uuid u = gen();

  return to_string(u);
}

template<typename T> std::string
save_archive (T const& s) {
  std::string fn = random<std::string>() +
    "-" BOOST_PP_STRINGIZE(BSL_TYPE)
    BOOST_PP_STRINGIZE(BSL_EXP(BSL_NODE_MAX, BSL_DEPTH))
    ".xml"
  ;

  std::ofstream ofs(fn.c_str());
 
  assert(ofs.good());
  
  xml_oarchive oa(ofs);
  oa << BOOST_SERIALIZATION_NVP(s);

  ofs.close();
  return fn;
}

template<typename T> std::pair<double, T>
restore_archive (std::string fn) {
  std::ifstream ifs(fn.c_str());
  T s;

  assert(ifs.good());
  
  high_resolution_timer u;
  
  xml_iarchive ia(ifs);
  ia >> BOOST_SERIALIZATION_NVP(s);

  ifs.close();
  return std::pair<double, T>(u.elapsed(), s);
}

class result_set_exception: public virtual archive_exception {
 public:
  enum exception_code {
    invalid_archive_metadata
  };

  result_set_exception (exception_code c = invalid_archive_metadata){ }
  
  virtual const char* what() const throw() {
    const char *msg = "";
    
    switch (code) {
      case invalid_archive_metadata:
        msg = "result set was not created on this system";
      default:
        archive_exception::what();
    }
    
    return msg;
  }
};

struct entry {
  std::string type;
  std::size_t size;
  double      data;

  template<class ARC>
  void serialize (ARC& ar, const unsigned int) {
    ar & BOOST_SERIALIZATION_NVP(type)
       & BOOST_SERIALIZATION_NVP(size)
       & BOOST_SERIALIZATION_NVP(data)
    ;
  }

  entry (void) { }

  entry (std::string type, std::size_t size, double data):
    type(type), size(size), data(data) { }
};

struct result_set {
  std::string      compiler;
  std::string      platform;
  std::list<entry> entries;

  template<class ARC>
  void serialize (ARC& ar, const unsigned int) {
    ar & BOOST_SERIALIZATION_NVP(compiler)
       & BOOST_SERIALIZATION_NVP(platform)
       & BOOST_SERIALIZATION_NVP(entries)
    ;

    if (  (compiler != BOOST_COMPILER)
       || (platform != BOOST_PLATFORM))
         throw result_set_exception(); 
  }

  result_set (void):
    compiler(BOOST_COMPILER),
    platform(BOOST_PLATFORM) { }

  result_set (std::list<entry> entries):
    compiler(BOOST_COMPILER),
    platform(BOOST_PLATFORM),
    entries(entries) { }
};

} // xml
} // archive
} // boost

#endif // BOOST_SERIALIZATION_XML_PERFORMANCE_HARNESS_HPP

