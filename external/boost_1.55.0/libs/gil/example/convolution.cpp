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
/// \brief Test file for convolve_rows() and convolve_cols() in the numeric extension
/// \author Lubomir Bourdev and Hailin Jin
/// \date February 27, 2007

#include <boost/gil/image.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/extension/io/jpeg_io.hpp>
#include <boost/gil/extension/numeric/kernel.hpp>
#include <boost/gil/extension/numeric/convolve.hpp>

int main() {
    using namespace boost::gil;

    rgb8_image_t img;
    jpeg_read_image("test.jpg",img);

    // Convolve the rows and the columns of the image with a fixed kernel
    rgb8_image_t convolved(img);

    // radius-1 Gaussian kernel, size 9
    float gaussian_1[]={0.00022923296f,0.0059770769f,0.060597949f,0.24173197f,0.38292751f,
                        0.24173197f,0.060597949f,0.0059770769f,0.00022923296f};
    /*
    // radius-2 Gaussian kernel, size 15
    float gaussian_2[]={
        0.00048869418f,0.0024031631f,0.0092463447f,
        0.027839607f,0.065602221f,0.12099898f,0.17469721f,
        0.19744757f,
        0.17469721f,0.12099898f,0.065602221f,0.027839607f,
        0.0092463447f,0.0024031631f,0.00048869418f
    };
    //radius-3 Gaussian kernel, size 23
    float gaussian_3[]={
        0.00016944126f,0.00053842377f,0.0015324751f,0.0039068931f,
        0.0089216027f,0.018248675f,0.033434924f,0.054872241f,
        0.080666073f,0.10622258f,0.12529446f,
        0.13238440f,
        0.12529446f,0.10622258f,0.080666073f,
        0.054872241f,0.033434924f,0.018248675f,0.0089216027f,
        0.0039068931f,0.0015324751f,0.00053842377f,0.00016944126f
    };
    //radius-4 Gaussian kernel, size 29
    float gaussian_4[]={
        0.00022466264f,0.00052009715f,0.0011314391f,0.0023129794f,
        0.0044433107f,0.0080211498f,0.013606987f,0.021691186f,
        0.032493830f,0.045742013f,0.060509924f,0.075220309f,
        0.087870099f,0.096459411f,0.099505201f,0.096459411f,0.087870099f,
        0.075220309f,0.060509924f,0.045742013f,0.032493830f,
        0.021691186f,0.013606987f,0.0080211498f,0.0044433107f,
        0.0023129794f,0.0011314391f,0.00052009715f,0.00022466264f,
    };
    */

    kernel_1d_fixed<float,9> kernel(gaussian_1,4);

    convolve_rows_fixed<rgb32f_pixel_t>(const_view(convolved),kernel,view(convolved));
    convolve_cols_fixed<rgb32f_pixel_t>(const_view(convolved),kernel,view(convolved));
    jpeg_write_view("out-convolution.jpg", view(convolved));

    // This is how to use a resizable kernel
    kernel_1d<float> kernel2(gaussian_1,9,4);
    convolve_rows<rgb32f_pixel_t>(const_view(img),kernel2,view(img));
    convolve_cols<rgb32f_pixel_t>(const_view(img),kernel2,view(img));
    jpeg_write_view("out-convolution2.jpg", view(img));

    return 0;
}
