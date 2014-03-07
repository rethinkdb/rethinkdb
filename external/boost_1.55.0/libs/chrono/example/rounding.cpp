//  french.cpp  ----------------------------------------------------------//

//  Copyright 2010 Howard Hinnant
//  Copyright 2011 Vicente J. Botet Escriba
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

// Adapted to Boost from the original Hawards's code

#include <iostream>
//#include <boost/chrono/chrono_io.hpp>
#include <boost/chrono/floor.hpp>
#include <boost/chrono/round.hpp>
#include <boost/chrono/ceil.hpp>

int main()
{
  boost::chrono::milliseconds ms(2500);
  std::cout << __FILE__ << "[" << __LINE__ << "]" << std::endl;
  std::cout << boost::chrono::floor<boost::chrono::seconds>(ms).count()
      << " seconds\n";
  std::cout << __FILE__ << "[" << __LINE__ << "]" << std::endl;
  std::cout << boost::chrono::round<boost::chrono::seconds>(ms).count()
      << " seconds\n";
  std::cout << __FILE__ << "[" << __LINE__ << "]" << std::endl;
  std::cout << boost::chrono::ceil<boost::chrono::seconds>(ms).count()
      << " seconds\n";
  std::cout << __FILE__ << "[" << __LINE__ << "]" << std::endl;
  ms = boost::chrono::milliseconds(2516);
  std::cout << __FILE__ << "[" << __LINE__ << "]" << std::endl;
  typedef boost::chrono::duration<long, boost::ratio<1, 30> > frame_rate;
  std::cout << __FILE__ << "[" << __LINE__ << "]" << std::endl;
  std::cout << boost::chrono::floor<frame_rate>(ms).count()
      << " [1/30] seconds\n";
  std::cout << __FILE__ << "[" << __LINE__ << "]" << std::endl;
  std::cout << boost::chrono::round<frame_rate>(ms).count()
      << " [1/30] seconds\n";
  std::cout << __FILE__ << "[" << __LINE__ << "]" << std::endl;
  std::cout << boost::chrono::ceil<frame_rate>(ms).count()
      << " [1/30] seconds\n";
  std::cout << __FILE__ << "[" << __LINE__ << "]" << std::endl;

  return 0;
}
