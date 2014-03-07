/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/

/*************************************************************************************************/

/// \file
/// \brief Test file for resize_view() in the numeric extension
/// \author Lubomir Bourdev and Hailin Jin
/// \date February 27, 2007

///////////////////////
////  NOTE: This sample file uses the numeric extension, which does not come with the Boost distribution.
////  You may download it from http://opensource.adobe.com/gil 
///////////////////////

#include <boost/gil/image.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/extension/io/jpeg_io.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>

int main() {
    using namespace boost::gil;

    rgb8_image_t img;
    jpeg_read_image("test.jpg",img);

    // test resize_view
    // Scale the image to 100x100 pixels using bilinear resampling
    rgb8_image_t square100x100(100,100);
    resize_view(const_view(img), view(square100x100), bilinear_sampler());
    jpeg_write_view("out-resize.jpg",const_view(square100x100));

    return 0;
}
