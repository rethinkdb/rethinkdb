/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/

/*************************************************************************************************/

/// \file
/// \brief Example file to show how to deal with packed pixels
/// \author Lubomir Bourdev and Hailin Jin
/// \date February 27, 2007
///
/// This test file demonstrates how to use packed pixel formats in GIL. 
/// A "packed" pixel is a pixel whose channels are bit ranges.
/// Here we create an RGB image whose pixel has 16-bits, such as:
/// bits [0..6] are the blue channel
/// bits [7..13] are the green channel
/// bits [14..15] are the red channel
/// We read a regular 8-bit RGB image, convert it to packed BGR772, convert it back to 8-bit RGB and save it to a file.
/// Since the red channel is only two bits the color loss should be observable in the result
///
/// This test file also demonstrates how to use bit-aligned images - these are images whose pixels themselves are not byte aligned.
/// For example, an rgb222 image has a pixel whose size is 6 bits. Bit-aligned images are more complicated than packed images. They
/// require a special proxy class to represent pixel reference and pixel iterator (packed images use C++ reference and C pointer respectively).
/// The alignment parameter in the constructor of bit-aligned images is in bit units. For example, if you want your bit-aligned image to have 4-byte
/// alignment of its rows use alignment of 32, not 4.
///
/// To demonstrate that image view transformations work on packed images, we save the result transposed.

#include <algorithm>
#include <boost/gil/extension/io/jpeg_io.hpp>

using namespace boost;
using namespace boost::gil;

int main() {
    bgr8_image_t img;
    jpeg_read_image("test.jpg",img);

    ////////////////////////////////
    // define a bgr772 image. It is a "packed" image - its channels are not byte-aligned, but its pixels are.
    ////////////////////////////////

    typedef packed_image3_type<uint16_t, 7,7,2, bgr_layout_t>::type bgr772_image_t;
    bgr772_image_t bgr772_img(img.dimensions());
    copy_and_convert_pixels(const_view(img),view(bgr772_img));

    // Save the result. JPEG I/O does not support the packed pixel format, so convert it back to 8-bit RGB
    jpeg_write_view("out-packed_pixel_bgr772.jpg",color_converted_view<bgr8_pixel_t>(transposed_view(const_view(bgr772_img))));

    ////////////////////////////////
    // define a gray1 image (one-bit per pixel). It is a "bit-aligned" image - its pixels are not byte aligned.
    ////////////////////////////////

    typedef bit_aligned_image1_type<1, gray_layout_t>::type gray1_image_t;
    gray1_image_t gray1_img(img.dimensions());
    copy_and_convert_pixels(const_view(img),view(gray1_img));

    // Save the result. JPEG I/O does not support the packed pixel format, so convert it back to 8-bit RGB
    jpeg_write_view("out-packed_pixel_gray1.jpg",color_converted_view<gray8_pixel_t>(transposed_view(const_view(gray1_img))));

    return 0;
}
