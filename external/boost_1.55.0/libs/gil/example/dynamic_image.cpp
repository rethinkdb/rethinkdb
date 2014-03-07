/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/

/*************************************************************************************************/

/// \file
/// \brief Test file for using dynamic images
/// \author Lubomir Bourdev and Hailin Jin
/// \date February 27, 2007

#include <boost/mpl/vector.hpp>
#include <boost/gil/extension/dynamic_image/any_image.hpp>
#include <boost/gil/extension/io/jpeg_dynamic_io.hpp>

int main() {
    using namespace boost::gil;

    typedef boost::mpl::vector<gray8_image_t, rgb8_image_t, gray16_image_t, rgb16_image_t> my_images_t;

    any_image<my_images_t> dynamic_img;
    jpeg_read_image("test.jpg",dynamic_img);

    // Save the image upside down, preserving its native color space and channel depth
    jpeg_write_view("out-dynamic_image.jpg",flipped_up_down_view(const_view(dynamic_img)));

    return 0;
}
