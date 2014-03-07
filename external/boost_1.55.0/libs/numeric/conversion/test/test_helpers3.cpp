// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


//
// NOTE: This file is intended to be used ONLY by the test files
//       from the Numeric Conversions Library
//

// The conversion test is performed using a class whose instances encapsulate
// a particular specific conversion defnied explicitely.
// A ConversionInstance object includes the source type, the target type,
// the source value and the expected result, including possible exceptions.
//

enum PostCondition { c_converted, c_overflow, c_neg_overflow, c_pos_overflow } ;

template<class Converter>
struct ConversionInstance
{
  typedef Converter converter ;

  typedef typename Converter::argument_type argument_type ;
  typedef typename Converter::result_type   result_type   ;

  typedef typename Converter::traits traits ;
  typedef typename traits::target_type target_type ;
  typedef typename traits::source_type source_type ;

  ConversionInstance ( result_type a_result, argument_type a_source, PostCondition a_post)
    :
    source(a_source),
    result(a_result),
    post(a_post)
  {}

  std::string to_string() const
    {
      return   std::string("converter<")
             + typeid(target_type).name()
             + std::string(",")
             + typeid(source_type).name()
             + std::string(">::convert(") ;
    }

  argument_type source ;
  result_type   result ;
  PostCondition post   ;
} ;

//
// Main conversion test point.
// Exercises a specific conversion described by 'conv'.
//
template<class Instance>
void test_conv_base( Instance const& conv )
{
  typedef typename Instance::argument_type argument_type ;
  typedef typename Instance::result_type   result_type   ;
  typedef typename Instance::converter     converter ;

  argument_type source = conv.source ;

  try
  {
    result_type result = converter::convert(source);

    if ( conv.post == c_converted )
    {
      BOOST_CHECK_MESSAGE( result == conv.result,
                           conv.to_string() <<  printable(source) << ")= " << printable(result) << ". Expected:" << printable(conv.result)
                         ) ;
    }
    else
    {
      BOOST_ERROR( conv.to_string() << printable(source) << ") = " << printable(result)
                   << ". Expected:" << ( conv.post == c_neg_overflow ? " negative_overflow" : "positive_overflow" )
                 ) ;
    }
  }
  catch ( boost::numeric::negative_overflow const& )
  {
    if ( conv.post == c_neg_overflow )
    {
      BOOST_CHECK_MESSAGE( true, conv.to_string() << printable(source) << ") = negative_overflow, as expected" ) ;
    }
    else
    {
      BOOST_ERROR( conv.to_string() << printable(source) << ") = negative_overflow. Expected:" <<  printable(conv.result) ) ;
    }
  }
  catch ( boost::numeric::positive_overflow const& )
  {
    if ( conv.post == c_pos_overflow )
    {
      BOOST_CHECK_MESSAGE( true, conv.to_string() << printable(source) << ") = positive_overflow, as expected" ) ;
    }
    else
    {
      BOOST_ERROR( conv.to_string() << printable(source) << ") = positive_overflow. Expected:" <<  printable(conv.result) ) ;
    }
  }
  catch ( boost::numeric::bad_numeric_cast const& )
  {
    if ( conv.post == c_overflow )
    {
      BOOST_CHECK_MESSAGE( true, conv.to_string() << printable(source) << ") = bad_numeric_cast, as expected" ) ;
    }
    else
    {
      BOOST_ERROR( conv.to_string() << printable(source) << ") = bad_numeric_cast. Expected:" <<  printable(conv.result) ) ;
    }
  }
}


#define TEST_SUCCEEDING_CONVERSION(Conv,typeT,typeS,valueT,valueS) \
        test_conv_base( ConversionInstance< Conv >(valueT, valueS, c_converted ) )

#define TEST_POS_OVERFLOW_CONVERSION(Conv,typeT,typeS,valueS) \
        test_conv_base( ConversionInstance< Conv >( static_cast< typeT >(0), valueS, c_pos_overflow ) )

#define TEST_NEG_OVERFLOW_CONVERSION(Conv,typeT,typeS,valueS) \
        test_conv_base( ConversionInstance< Conv >( static_cast< typeT >(0), valueS, c_neg_overflow ) )

#define DEF_CONVERTER(T,S) boost::numeric::converter< T , S >

#define TEST_SUCCEEDING_CONVERSION_DEF(typeT,typeS,valueT,valueS) \
        TEST_SUCCEEDING_CONVERSION( DEF_CONVERTER(typeT,typeS), typeT, typeS, valueT, valueS )

#define TEST_POS_OVERFLOW_CONVERSION_DEF(typeT,typeS,valueS) \
        TEST_POS_OVERFLOW_CONVERSION( DEF_CONVERTER(typeT,typeS), typeT, typeS, valueS )

#define TEST_NEG_OVERFLOW_CONVERSION_DEF(typeT,typeS,valueS) \
        TEST_NEG_OVERFLOW_CONVERSION( DEF_CONVERTER(typeT,typeS), typeT, typeS, valueS )


//
///////////////////////////////////////////////////////////////////////////////////////////////

