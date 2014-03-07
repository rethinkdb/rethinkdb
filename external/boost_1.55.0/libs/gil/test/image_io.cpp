/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/

#include <boost/mpl/vector.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/extension/dynamic_image/any_image.hpp>
#include <boost/gil/image_view.hpp>
#include <boost/gil/planar_pixel_reference.hpp>
#include <boost/gil/color_convert.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/image_view_factory.hpp>
#ifndef BOOST_GIL_NO_IO
#include <boost/gil/extension/io/tiff_io.hpp>
#include <boost/gil/extension/io/jpeg_io.hpp>
#include <boost/gil/extension/io/png_io.hpp>
#include <boost/gil/extension/io/tiff_dynamic_io.hpp>
#include <boost/gil/extension/io/jpeg_dynamic_io.hpp>
#include <boost/gil/extension/io/png_dynamic_io.hpp>
#endif

using namespace boost::gil;

typedef any_image<boost::mpl::vector<gray8_image_t, bgr8_image_t, argb8_image_t,
                                     rgb8_image_t, rgb8_planar_image_t, 
                                     cmyk8_image_t, cmyk8_planar_image_t, 
                                     rgba8_image_t, rgba8_planar_image_t> > any_image_t;

#ifdef BOOST_GIL_NO_IO
void test_image_io() {} // IO is not tested when BOOST_GIL_NO_IO is enabled
#else 
void test_image_io() {
    const std::string in_dir="";  // directory of source images
    const std::string out_dir=in_dir+"image_io-out\\";
// *********************************** 
// ************************ GRAY IMAGE
// *********************************** 
    gray8_image_t imgGray;
// TIFF
    // load gray tiff into gray image
    tiff_read_image(in_dir+"gray.tif",imgGray);
    // save gray image to tiff
    tiff_write_view(out_dir+"grayFromGray.tif",view(imgGray));

    // load RGB tiff into gray image
    tiff_read_and_convert_image(in_dir+"RGB.tif",imgGray);

    // save gray image to tiff (again!)
    tiff_write_view(out_dir+"grayFromRGB.tif",view(imgGray));

// JPEG
    // load gray jpeg into gray image
    jpeg_read_image(in_dir+"gray.jpg",imgGray);
    // save gray image to gray jpeg
    jpeg_write_view(out_dir+"grayFromGray.jpg",view(imgGray));
    
    // load RGB jpeg into gray image
    jpeg_read_and_convert_image(in_dir+"RGB.jpg",imgGray);
    // save gray image to RGB jpeg
    jpeg_write_view(out_dir+"grayFromRGB.jpg",color_converted_view<rgb8_pixel_t>(view(imgGray)));

// PNG
    // load gray png into gray image
    png_read_image(in_dir+"gray.png",imgGray);
    // save gray image to gray png
    png_write_view(out_dir+"grayFromGray.png",view(imgGray));
    
    // load RGB png into gray image
    png_read_and_convert_image(in_dir+"RGB.png",imgGray);
    // save gray image to RGB png
    png_write_view(out_dir+"grayFromRGB.png",color_converted_view<rgb8_pixel_t>(view(imgGray)));

// *********************************** 
// ************************* RGB Planar
// *********************************** 

    rgb8_image_t imgRGB;

// TIFF

    // load gray tiff into RGB image
    tiff_read_and_convert_image(in_dir+"gray.tif",imgRGB);
    // save RGB image to tiff
    tiff_write_view(out_dir+"RGBFromGray.tif",view(imgRGB));

    // load RGB tiff into RGB image
    tiff_read_image(in_dir+"RGB.tif",imgRGB);
    // save RGB image to tiff (again!)
    tiff_write_view(out_dir+"RGBFromRGB.tif",view(imgRGB));

// JPEG
    // load gray jpeg into RGB image
    jpeg_read_and_convert_image(in_dir+"gray.jpg",imgRGB);
    // save RGB image to gray jpeg
    jpeg_write_view(out_dir+"RGBFromGray.jpg",view(imgRGB));
    
    // load RGB jpeg into RGB image
    jpeg_read_image(in_dir+"RGB.jpg",imgRGB);
    // save RGB image to RGB jpeg
    jpeg_write_view(out_dir+"RGBFromRGB.jpg",view(imgRGB));

// PNG
    // load gray png into RGB image
    png_read_and_convert_image(in_dir+"gray.png",imgRGB);
    // save RGB image to gray png
    png_write_view(out_dir+"RGBFromGray.png",view(imgRGB));
    
    // load RGB png into RGB image
    png_read_image(in_dir+"RGB.png",imgRGB);
    // save RGB image to RGB png
    png_write_view(out_dir+"RGBFromRGB.png",view(imgRGB));

// *********************************** 
// ************************ GRAY32 Planar
// *********************************** 
    gray32f_image_t imgGray32;
// TIFF
    // load gray tiff into gray image
    tiff_read_and_convert_image(in_dir+"gray.tif",imgGray32);
    // save gray image to tiff
    tiff_write_view(out_dir+"gray32FromGray.tif",view(imgGray32));
        
    // load RGB tiff into gray image
    tiff_read_and_convert_image(in_dir+"RGB.tif",imgGray32);

    // save gray image to tiff (again!)
    tiff_write_view(out_dir+"gray32FromRGB.tif",view(imgGray32));

// JPEG
    tiff_read_and_convert_image(in_dir+"gray.tif",imgGray32);    // again TIF (jpeg load not supported)
    // save RGB image to gray jpeg
    tiff_write_view(out_dir+"gray32FromGray.jpg",view(imgGray32));
    
    // load RGB jpeg into RGB image
    tiff_read_and_convert_image(in_dir+"RGB.tif",imgGray32);    // again TIF (jpeg load not supported)
    // save RGB image to RGB jpeg
    tiff_write_view(out_dir+"gray32FromRGB.jpg",color_converted_view<rgb32f_pixel_t>(view(imgGray32)));

// *********************************** 
// ************************ NATIVE Planar
// *********************************** 
    any_image_t anyImg;

// TIFF
    // load RGB tiff into any image
    tiff_read_image(in_dir+"RGB.tif",anyImg);

    // save any image to tiff
    tiff_write_view(out_dir+"RGBNative.tif",view(anyImg));

    // load gray tiff into any image
    tiff_read_image(in_dir+"gray.tif",anyImg);
    
    // save any image to tiff
    tiff_write_view(out_dir+"grayNative.tif",view(anyImg));

// JPEG
    // load gray jpeg into any image
    jpeg_read_image(in_dir+"gray.jpg",anyImg);
    // save any image to jpeg
    jpeg_write_view(out_dir+"grayNative.jpg",view(anyImg));
    
    // load RGB jpeg into any image
    jpeg_read_image(in_dir+"RGB.jpg",anyImg);
    // save any image to jpeg
    jpeg_write_view(out_dir+"RGBNative.jpg",view(anyImg));

// PNG
    // load gray png into any image
    png_read_image(in_dir+"gray.png",anyImg);
    // save any image to png
    png_write_view(out_dir+"grayNative.png",view(anyImg));
    
    // load RGB png into any image
    png_read_image(in_dir+"RGB.png",anyImg);
    // save any image to png
    png_write_view(out_dir+"RGBNative.png",view(anyImg));
}
#endif

int main(int argc, char* argv[]) {
    test_image_io();
    return 0;
}

