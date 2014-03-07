/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/

/*************************************************************************************************/

///////////////////////
////  NOTE: This sample file uses the numeric extension, which does not come with the Boost distribution.
////  You may download it from http://opensource.adobe.com/gil 
///////////////////////

/// \file
/// \brief Test file for resample_pixels() in the numeric extension
/// \author Lubomir Bourdev and Hailin Jin
/// \date February 27, 2007

#include <boost/gil/image.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/extension/io/jpeg_io.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>

int main() {
    using namespace boost::gil;

    rgb8_image_t img;
    jpeg_read_image("test.jpg",img);

    // test resample_pixels
    // Transform the image by an arbitrary affine transformation using nearest-neighbor resampling
    rgb8_image_t transf(rgb8_image_t::point_t(view(img).dimensions()*2));
    fill_pixels(view(transf),rgb8_pixel_t(255,0,0));    // the background is red

    matrix3x2<double> mat = matrix3x2<double>::get_translate(-point2<double>(200,250)) *
                            matrix3x2<double>::get_rotate(-15*3.14/180.0);
    resample_pixels(const_view(img), view(transf), mat, nearest_neighbor_sampler());
    jpeg_write_view("out-affine.jpg", view(transf));

    return 0;
}
