/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/

/*************************************************************************************************/

/// \file
/// \brief Example file to demonstrate how to create a model of a pixel iterator
/// \author Lubomir Bourdev and Hailin Jin
/// \date February 27, 2007

#ifdef WIN32
#define _CRT_SECURE_NO_DEPRECATE 1
#pragma warning(disable : 4244) // 
#pragma warning(disable : 4996) // MSFT declared it deprecated
#endif

// gcc doesn't compile unless we forward-declare at_c before we include gil...
namespace boost { namespace gil {
    template <typename ChannelReference, typename Layout> struct interleaved_ref;
    template <typename ColorBase> struct element_reference_type;

    template <int K, typename ChannelReference, typename Layout>
    typename element_reference_type<interleaved_ref<ChannelReference,Layout> >::type
    at_c(const interleaved_ref<ChannelReference,Layout>& p);
} }

#include <iostream>
#include <boost/gil/extension/io/jpeg_dynamic_io.hpp>
#include "interleaved_ptr.hpp"

int main(int argc, unsigned char* argv[])
{
    using namespace boost::gil;

    typedef interleaved_ptr<unsigned char*, rgb_layout_t> rgb8_interleaved_ptr;
    typedef interleaved_ptr<const unsigned char*, rgb_layout_t> rgb8c_interleaved_ptr;

    boost::function_requires<MutablePixelIteratorConcept<rgb8_interleaved_ptr> >();
    boost::function_requires<PixelIteratorConcept<rgb8c_interleaved_ptr> >();
    boost::function_requires<MemoryBasedIteratorConcept<memory_based_step_iterator<rgb8_interleaved_ptr> > >();

    boost::function_requires<MutablePixelConcept<rgb8_interleaved_ptr::value_type> >();
    boost::function_requires<PixelConcept<rgb8c_interleaved_ptr::value_type> >();

    typedef type_from_x_iterator<rgb8_interleaved_ptr >::view_t rgb8_interleaved_view_t;
    typedef type_from_x_iterator<rgb8c_interleaved_ptr>::view_t rgb8c_interleaved_view_t;

    boost::function_requires<MutableImageViewConcept<rgb8_interleaved_view_t> >();
    boost::function_requires<ImageViewConcept<rgb8c_interleaved_view_t> >();

    rgb8_image_t img;
    jpeg_read_image("test.jpg", img);

    // Get a raw pointer to the RGB buffer
    unsigned char* raw_ptr=&view(img)[0][0];

    // Construct a view from it, without casting it to rgb8_pixel_t*
    rgb8_interleaved_view_t src_view=interleaved_view(img.width(),img.height(),rgb8_interleaved_ptr(raw_ptr),
                                                      view(img).pixels().row_size());

    // Apply view transformations and algorithms on it
    jpeg_write_view("out-interleaved_ptr.jpg",nth_channel_view(flipped_up_down_view(src_view),1));

    return 0;
}


