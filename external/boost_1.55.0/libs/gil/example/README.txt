Copyright 2008 Lubomir Bourdev and Hailin Jin

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)

This directory contains GIL sample code.

We provide a Makefile that compiles all examples. You will need to change it to specify the correct path to boost, gil, and libjpeg. Some of the examples include the GIL numeric extension, which you can get from:
http://opensource.adobe.com/gil/download.html

The makefile generates a separate executable for each test file. Each executable generates its output as "out-<example_name>.jpg". For example, the resize.cpp example generates the image out-resize.jpg

The following examples are included:

1. resize.cpp
   Scales an image using bilinear or nearest-neighbor resampling

2. affine.cpp
   Performs an arbitrary affine transformation on the image

3. convolution.cpp
   Convolves the image with a Gaussian kernel

4. mandelbrot.cpp
   Creates a synthetic image defining the Mandelbrot set

5. interleaved_ptr.cpp
   Illustrates how to create a custom pixel reference and iterator.
   Creates a GIL image view over user-supplied data without the need to cast to GIL pixel type

6. x_gradient.cpp
   Horizontal gradient, from the tutorial

7. histogram.cpp
   Algorithm to compute the histogram of an image

8. packed_pixel.cpp
   Illustrates how to create a custom pixel model - a pixel whose channel size is not divisible by bytes

9. dynamic_image.cpp
   Example of using images whose type is instantiated at run time


