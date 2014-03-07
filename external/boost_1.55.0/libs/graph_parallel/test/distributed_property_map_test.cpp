// Copyright (C) 2004-2008 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/test/minimal.hpp>
#include <vector>
#include <string>
#include <boost/serialization/string.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/parallel/basic_reduce.hpp>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif

using namespace boost;
using boost::graph::distributed::mpi_process_group;

enum color_t { red, blue };

struct remote_key 
{
  remote_key(int p = -1, std::size_t l = 0) : processor(p), local_key(l) {}

  int processor;
  std::size_t local_key;

  template<typename Archiver>
  void serialize(Archiver& ar, const unsigned int /*version*/)
  {
    ar & processor & local_key;
  }
};

namespace boost { namespace mpi {
    template<> struct is_mpi_datatype<remote_key> : mpl::true_ { };
} }
BOOST_IS_BITWISE_SERIALIZABLE(remote_key)
BOOST_CLASS_IMPLEMENTATION(remote_key,object_serializable)
BOOST_CLASS_TRACKING(remote_key,track_never)

namespace boost { 

template<>
struct hash<remote_key>
{
  std::size_t operator()(const remote_key& key) const
  {
    std::size_t hash = hash_value(key.processor);
    hash_combine(hash, key.local_key);
    return hash;
  }
};
}

inline bool operator==(const remote_key& x, const remote_key& y)
{ return x.processor == y.processor && x.local_key == y.local_key; }

struct remote_key_to_global
{
  typedef readable_property_map_tag category;
  typedef remote_key key_type;
  typedef std::pair<int, std::size_t> value_type;
  typedef value_type reference;
};

inline std::pair<int, std::size_t> 
get(remote_key_to_global, const remote_key& key)
{
  return std::make_pair(key.processor, key.local_key);
}

template<typename T>
struct my_reduce : boost::parallel::basic_reduce<T> {
  BOOST_STATIC_CONSTANT(bool, non_default_resolver = true);
};

void colored_test()
{
  mpi_process_group pg;
  const int n = 500;
  
  color_t my_start_color = process_id(pg) % 2 == 0? ::red : ::blue;
  int next_processor = (process_id(pg) + 1) % num_processes(pg);
  color_t next_start_color = next_processor % 2 == 0? ::red : ::blue;

  // Initial color map: even-numbered processes are all red,
  // odd-numbered processes are all blue.
  std::vector<color_t> color_vec(n, my_start_color);

  typedef iterator_property_map<std::vector<color_t>::iterator, 
                                identity_property_map> LocalPropertyMap;
  LocalPropertyMap local_colors(color_vec.begin(), identity_property_map());

  synchronize(pg);

  // Create the distributed property map
  typedef boost::parallel::distributed_property_map<mpi_process_group,
                                                    remote_key_to_global,
                                                    LocalPropertyMap> ColorMap;
  ColorMap colors(pg, remote_key_to_global(), local_colors);
  colors.set_reduce(my_reduce<color_t>());

  if (process_id(pg) == 0) std::cerr << "Checking local colors...";
  // check local processor colors
  for (int i = 0; i < n; ++i) {
    remote_key k(process_id(pg), i);
    BOOST_CHECK(get(colors, k) == my_start_color);
  }

  colors.set_consistency_model(boost::parallel::cm_bidirectional);
  if (process_id(pg) == 0) std::cerr << "OK.\nChecking next processor's default colors...";
  // check next processor's colors
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    BOOST_CHECK(get(colors, k) == color_t());
  }

  if (process_id(pg) == 0) std::cerr << "OK.\nSynchronizing...";
  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\nChecking next processor's colors...";
  // check next processor's colors
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    BOOST_CHECK(get(colors, k) == next_start_color);
  }

  if (process_id(pg) == 0) std::cerr << "OK.\nSynchronizing...";
  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\nChanging next processor's colors...";
  // change the next processor's colors
  color_t next_finish_color = next_processor % 2 == 0? ::blue : ::red;
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    put(colors, k, next_finish_color);
  }

  if (process_id(pg) == 0) std::cerr << "OK.\nSynchronizing...";
  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\nChecking changed colors...";
  // check our own colors
  color_t my_finish_color = process_id(pg) % 2 == 0? ::blue : ::red;
  for (int i = 0; i < n; ++i) {
    remote_key k(process_id(pg), i);
    BOOST_CHECK(get(colors, k) == my_finish_color);
  }

  // check our neighbor's colors
  if (process_id(pg) == 0) std::cerr << "OK.\nChecking changed colors on neighbor...";
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    BOOST_CHECK(get(colors, k) == next_finish_color);
  }

  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\n";
}

void bool_test()
{
  mpi_process_group pg;
  const int n = 500;
  
  bool my_start_value = process_id(pg) % 2;
  int next_processor = (process_id(pg) + 1) % num_processes(pg);
  bool next_start_value = ((process_id(pg) + 1) % num_processes(pg)) % 2;

  // Initial color map: even-numbered processes are false, 
  // odd-numbered processes are true
  std::vector<bool> bool_vec(n, my_start_value);

  typedef iterator_property_map<std::vector<bool>::iterator, 
                                identity_property_map> LocalPropertyMap;
  LocalPropertyMap local_values(bool_vec.begin(), identity_property_map());

  synchronize(pg);

  // Create the distributed property map
  typedef boost::parallel::distributed_property_map<mpi_process_group,
                                                    remote_key_to_global,
                                                    LocalPropertyMap> ValueMap;
  ValueMap values(pg, remote_key_to_global(), local_values);
  values.set_reduce(my_reduce<bool>());

  if (process_id(pg) == 0) std::cerr << "Checking local values...";
  // check local processor values
  for (int i = 0; i < n; ++i) {
    remote_key k(process_id(pg), i);
    BOOST_CHECK(get(values, k) == my_start_value);
  }

  values.set_consistency_model(boost::parallel::cm_bidirectional);
  if (process_id(pg) == 0) std::cerr << "OK.\nChecking next processor's default values...";
  // check next processor's values
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    BOOST_CHECK(get(values, k) == false);
  }

  if (process_id(pg) == 0) std::cerr << "OK.\nSynchronizing...";
  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\nChecking next processor's values...";
  // check next processor's values
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    BOOST_CHECK(get(values, k) == next_start_value);
  }

  if (process_id(pg) == 0) std::cerr << "OK.\nSynchronizing...";
  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\nChanging next processor's values...";
  // change the next processor's values
  bool next_finish_value = next_processor % 2 == 0;
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    put(values, k, next_finish_value);
  }

  if (process_id(pg) == 0) std::cerr << "OK.\nSynchronizing...";
  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\nChecking changed values...";
  // check our own values
  bool my_finish_value = process_id(pg) % 2 == 0;
  for (int i = 0; i < n; ++i) {
    remote_key k(process_id(pg), i);
    BOOST_CHECK(get(values, k) == my_finish_value);
  }

  // check our neighbor's values
  if (process_id(pg) == 0) std::cerr << "OK.\nChecking changed values on neighbor...";
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    BOOST_CHECK(get(values, k) == next_finish_value);
  }

  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\n";
}

void string_test()
{
  mpi_process_group pg;
  const int n = 500;
  
  std::string my_start_string = lexical_cast<std::string>(process_id(pg));
  int next_processor = (process_id(pg) + 1) % num_processes(pg);
  std::string next_start_string = lexical_cast<std::string>(next_processor);

  // Initial color map: even-numbered processes are false, 
  // odd-numbered processes are true
  std::vector<std::string> string_vec(n, my_start_string);

  typedef iterator_property_map<std::vector<std::string>::iterator, 
                                identity_property_map> LocalPropertyMap;
  LocalPropertyMap local_strings(string_vec.begin(), identity_property_map());

  synchronize(pg);

  // Create the distributed property map
  typedef boost::parallel::distributed_property_map<mpi_process_group,
                                                    remote_key_to_global,
                                                    LocalPropertyMap> StringMap;
  StringMap strings(pg, remote_key_to_global(), local_strings);
  strings.set_reduce(my_reduce<std::string>());

  if (process_id(pg) == 0) std::cerr << "Checking local strings...";
  // check local processor strings
  for (int i = 0; i < n; ++i) {
    remote_key k(process_id(pg), i);
    BOOST_CHECK(get(strings, k) == my_start_string);
  }

  strings.set_consistency_model(boost::parallel::cm_bidirectional);
  if (process_id(pg) == 0) std::cerr << "OK.\nChecking next processor's default strings...";
  // check next processor's strings
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    BOOST_CHECK(get(strings, k) == (num_processes(pg) == 1 ? my_start_string : std::string()));
  }

  if (process_id(pg) == 0) std::cerr << "OK.\nSynchronizing...";
  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\nChecking next processor's strings...";
  // check next processor's strings
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    BOOST_CHECK(get(strings, k) == next_start_string);
  }

  if (process_id(pg) == 0) std::cerr << "OK.\nSynchronizing...";
  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\nChanging next processor's strings...";
  // change the next processor's strings
  std::string next_finish_string = next_start_string + next_start_string;
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    put(strings, k, next_finish_string);
  }

  if (process_id(pg) == 0) std::cerr << "OK.\nSynchronizing...";
  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\nChecking changed strings...";
  // check our own strings
  std::string my_finish_string = my_start_string + my_start_string;
  for (int i = 0; i < n; ++i) {
    remote_key k(process_id(pg), i);
    BOOST_CHECK(get(strings, k) == my_finish_string);
  }

  // check our neighbor's strings
  if (process_id(pg) == 0) std::cerr << "OK.\nChecking changed strings on neighbor...";
  for (int i = 0; i < n; ++i) {
    remote_key k(next_processor, i);
    BOOST_CHECK(get(strings, k) == next_finish_string);
  }

  synchronize(pg);

  if (process_id(pg) == 0) std::cerr << "OK.\n";
}

int test_main(int argc, char** argv)
{
  boost::mpi::environment env(argc, argv);
  colored_test();
  bool_test();
  string_test();
  return 0;
}
