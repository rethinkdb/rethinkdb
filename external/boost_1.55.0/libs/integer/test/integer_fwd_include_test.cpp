//  Copyright John Maddock 2009.  
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/integer_fwd.hpp> // must be the only #include!

// just declare some functions that use the incomplete types in the header:

void f1(const boost::integer_traits<char>*);
void f2(const boost::int_fast_t<char>*);
void f3(const boost::int_t<12>*);
void f4(const boost::uint_t<31>*);
void f5(const boost::int_max_value_t<100>*);
void f6(const boost::int_min_value_t<-100>*);
void f7(const boost::uint_value_t<100>*);
void f8(const boost::high_bit_mask_t<10>*);
void f9(const boost::low_bits_mask_t<10>*);
void f10(boost::static_log2_argument_type, boost::static_log2_result_type, boost::static_log2<10>*);
void f11(boost::static_min_max_signed_type, boost::static_min_max_unsigned_type);
void f12(boost::static_signed_min<1, 2>*, boost::static_signed_max<1,2>*);
void f13(boost::static_unsigned_min<1,2>*, boost::static_unsigned_min<1,2>*);
