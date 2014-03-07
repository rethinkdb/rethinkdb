/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/

/*************************************************************************************************/

#ifndef GIL_IMAGE_VIEW_FACTORY_HPP
#define GIL_IMAGE_VIEW_FACTORY_HPP

/*!
/// \file               
/// \brief Methods for constructing image views from raw data or other image views
/// \author Lubomir Bourdev and Hailin Jin \n
///         Adobe Systems Incorporated
/// \date   2005-2007 \n Last updated on March 9, 2007
/// Methods for creating shallow image views from raw pixel data or from other image views - 
/// flipping horizontally or vertically, axis-aligned rotation, a subimage, subsampled 
/// or n-th channel image view. Derived image views are shallow copies and are fast to construct.
*/

#include <cassert>
#include <cstddef>
#include "gil_config.hpp"
#include "metafunctions.hpp"
#include "gray.hpp"
#include "color_convert.hpp"

/// \defgroup ImageViewConstructors Image View From Raw Data
/// \ingroup ImageViewAlgorithm
/// \brief Methods for constructing image views from raw data and for getting raw data from views

/// \defgroup ImageViewTransformations Image View Transformations
/// \ingroup ImageViewAlgorithm
/// \brief Methods for constructing one image view from another

namespace boost { namespace gil {
struct default_color_converter;

template <typename T> struct dynamic_x_step_type;
template <typename T> struct dynamic_y_step_type;
template <typename T> struct transposed_type;

/// \brief Returns the type of a view that has a dynamic step along both X and Y
/// \ingroup ImageViewTransformations
template <typename View> 
struct dynamic_xy_step_type : public dynamic_y_step_type<typename dynamic_x_step_type<View>::type> {};

/// \brief Returns the type of a transposed view that has a dynamic step along both X and Y
/// \ingroup ImageViewTransformations
template <typename View> 
struct dynamic_xy_step_transposed_type : public dynamic_xy_step_type<typename transposed_type<View>::type> {};


/// \ingroup ImageViewConstructors
/// \brief Constructing image views from raw interleaved pixel data
template <typename Iterator>
typename type_from_x_iterator<Iterator>::view_t
interleaved_view(std::size_t width, std::size_t height,
                 Iterator pixels, std::ptrdiff_t rowsize_in_bytes) {
    typedef typename type_from_x_iterator<Iterator>::view_t RView;
    return RView(width, height, typename RView::locator(pixels, rowsize_in_bytes));
}

/// \ingroup ImageViewConstructors
/// \brief Constructing image views from raw interleaved pixel data
template <typename Iterator>
typename type_from_x_iterator<Iterator>::view_t
interleaved_view(point2<std::size_t> dim,
                 Iterator pixels, std::ptrdiff_t rowsize_in_bytes) {
    typedef typename type_from_x_iterator<Iterator>::view_t RView;
    return RView(dim, typename RView::locator(pixels, rowsize_in_bytes));
}

/////////////////////////////
//  interleaved_view_get_raw_data, planar_view_get_raw_data - return pointers to the raw data (the channels) of a basic homogeneous view.
/////////////////////////////

namespace detail {
    template <typename View, bool IsMutable> struct channel_pointer_type_impl;

    template <typename View> struct channel_pointer_type_impl<View, true> {
        typedef       typename channel_type<View>::type* type;
    };
    template <typename View> struct channel_pointer_type_impl<View, false> {
        typedef const typename channel_type<View>::type* type;
    };

    template <typename View> struct channel_pointer_type
        : public channel_pointer_type_impl<View, view_is_mutable<View>::value> {};
} // namespace detail

/// \ingroup ImageViewConstructors
/// \brief Returns C pointer to the the channels of an interleaved homogeneous view.
template <typename HomogeneousView>
typename detail::channel_pointer_type<HomogeneousView>::type interleaved_view_get_raw_data(const HomogeneousView& view) {
    BOOST_STATIC_ASSERT((!is_planar<HomogeneousView>::value && view_is_basic<HomogeneousView>::value));
    BOOST_STATIC_ASSERT((boost::is_pointer<typename HomogeneousView::x_iterator>::value));

    return &gil::at_c<0>(view(0,0));
}

/// \ingroup ImageViewConstructors
/// \brief Returns C pointer to the the channels of a given color plane of a planar homogeneous view.
template <typename HomogeneousView>
typename detail::channel_pointer_type<HomogeneousView>::type planar_view_get_raw_data(const HomogeneousView& view, int plane_index) {
    BOOST_STATIC_ASSERT((is_planar<HomogeneousView>::value && view_is_basic<HomogeneousView>::value));
    return dynamic_at_c(view.row_begin(0),plane_index);
}


/// \defgroup ImageViewTransformationsColorConvert color_converted_view
/// \ingroup ImageViewTransformations
/// \brief Color converted view of another view

/// \ingroup ImageViewTransformationsColorConvert PixelDereferenceAdaptorModel
/// \brief Function object that given a source pixel, returns it converted to a given color space and channel depth. Models: PixelDereferenceAdaptorConcept
///
/// Useful in constructing a color converted view over a given image view
template <typename SrcConstRefP, typename DstP, typename CC=default_color_converter >        // const_reference to the source pixel and destination pixel value
class color_convert_deref_fn : public deref_base<color_convert_deref_fn<SrcConstRefP,DstP,CC>, DstP, DstP, const DstP&, SrcConstRefP, DstP, false> {
private:
    CC _cc;                     // color-converter
public:
    color_convert_deref_fn() {}
    color_convert_deref_fn(CC cc_in) : _cc(cc_in) {}

    DstP operator()(SrcConstRefP srcP) const {
        DstP dstP;
        _cc(srcP,dstP);
        return dstP;
    }
};

namespace detail {
    // Add color converter upon dereferencing
    template <typename SrcView, typename CC, typename DstP, typename SrcP>
    struct _color_converted_view_type {
    private:
        typedef color_convert_deref_fn<typename SrcView::const_t::reference,DstP,CC> deref_t;
        typedef typename SrcView::template add_deref<deref_t> add_ref_t;
    public:
        typedef typename add_ref_t::type type;
        static type make(const SrcView& sv,CC cc) {return add_ref_t::make(sv,deref_t(cc));}
    };

    // If the Src view has the same pixel type as the target, there is no need for color conversion
    template <typename SrcView, typename CC, typename DstP>
    struct _color_converted_view_type<SrcView,CC,DstP,DstP> {
        typedef SrcView type;
        static type make(const SrcView& sv,CC) {return sv;}
    };
} // namespace detail


/// \brief Returns the type of a view that does color conversion upon dereferencing its pixels 
/// \ingroup ImageViewTransformationsColorConvert
template <typename SrcView, typename DstP, typename CC=default_color_converter>
struct color_converted_view_type : public detail::_color_converted_view_type<SrcView,
                                                                             CC,
                                                                             DstP,
                                                                             typename SrcView::value_type> {
    GIL_CLASS_REQUIRE(DstP, boost::gil, MutablePixelConcept)//why does it have to be mutable???
};


/// \ingroup ImageViewTransformationsColorConvert
/// \brief view of a different color space with a user defined color-converter
template <typename DstP, typename View, typename CC>
inline typename color_converted_view_type<View,DstP,CC>::type color_converted_view(const View& src,CC cc) {
    return color_converted_view_type<View,DstP,CC>::make(src,cc);
}

/// \ingroup ImageViewTransformationsColorConvert
/// \brief overload of generic color_converted_view with the default color-converter
template <typename DstP, typename View>
inline typename color_converted_view_type<View,DstP>::type
color_converted_view(const View& src) {
    return color_converted_view<DstP>(src,default_color_converter());
}

/// \defgroup ImageViewTransformationsFlipUD flipped_up_down_view
/// \ingroup ImageViewTransformations
/// \brief view of a view flipped up-to-down

/// \ingroup ImageViewTransformationsFlipUD
template <typename View>
inline typename dynamic_y_step_type<View>::type flipped_up_down_view(const View& src) { 
    typedef typename dynamic_y_step_type<View>::type RView;
    return RView(src.dimensions(),typename RView::xy_locator(src.xy_at(0,src.height()-1),-1));
}

/// \defgroup ImageViewTransformationsFlipLR flipped_left_right_view
/// \ingroup ImageViewTransformations
/// \brief view of a view flipped left-to-right

/// \ingroup ImageViewTransformationsFlipLR
template <typename View> 
inline typename dynamic_x_step_type<View>::type flipped_left_right_view(const View& src) {
    typedef typename dynamic_x_step_type<View>::type RView;
    return RView(src.dimensions(),typename RView::xy_locator(src.xy_at(src.width()-1,0),-1,1));
}

/// \defgroup ImageViewTransformationsTransposed transposed_view
/// \ingroup ImageViewTransformations
/// \brief view of a view transposed

/// \ingroup ImageViewTransformationsTransposed
template <typename View>
inline typename dynamic_xy_step_transposed_type<View>::type transposed_view(const View& src) {
    typedef typename dynamic_xy_step_transposed_type<View>::type RView;
    return RView(src.height(),src.width(),typename RView::xy_locator(src.xy_at(0,0),1,1,true));
}

/// \defgroup ImageViewTransformations90CW rotated90cw_view
/// \ingroup ImageViewTransformations
/// \brief view of a view rotated 90 degrees clockwise

/// \ingroup ImageViewTransformations90CW
template <typename View> 
inline typename dynamic_xy_step_transposed_type<View>::type rotated90cw_view(const View& src) {
    typedef typename dynamic_xy_step_transposed_type<View>::type RView;
    return RView(src.height(),src.width(),typename RView::xy_locator(src.xy_at(0,src.height()-1),-1,1,true));
}

/// \defgroup ImageViewTransformations90CCW rotated90ccw_view
/// \ingroup ImageViewTransformations
/// \brief view of a view rotated 90 degrees counter-clockwise

/// \ingroup ImageViewTransformations90CCW
template <typename View> 
inline typename dynamic_xy_step_transposed_type<View>::type rotated90ccw_view(const View& src) {
    typedef typename dynamic_xy_step_transposed_type<View>::type RView;
    return RView(src.height(),src.width(),typename RView::xy_locator(src.xy_at(src.width()-1,0),1,-1,true));
}

/// \defgroup ImageViewTransformations180 rotated180_view
/// \ingroup ImageViewTransformations
/// \brief view of a view rotated 180 degrees

/// \ingroup ImageViewTransformations180
template <typename View> 
inline typename dynamic_xy_step_type<View>::type rotated180_view(const View& src) {
    typedef typename dynamic_xy_step_type<View>::type RView;
    return RView(src.dimensions(),typename RView::xy_locator(src.xy_at(src.width()-1,src.height()-1),-1,-1));
}

/// \defgroup ImageViewTransformationsSubimage subimage_view
/// \ingroup ImageViewTransformations
/// \brief view of an axis-aligned rectangular area within an image_view

/// \ingroup ImageViewTransformationsSubimage
template <typename View> 
inline View subimage_view(const View& src, const typename View::point_t& topleft, const typename View::point_t& dimensions) {
    return View(dimensions,src.xy_at(topleft));
}

/// \ingroup ImageViewTransformationsSubimage
template <typename View> 
inline View subimage_view(const View& src, int xMin, int yMin, int width, int height) {
    return View(width,height,src.xy_at(xMin,yMin));
}

/// \defgroup ImageViewTransformationsSubsampled subsampled_view
/// \ingroup ImageViewTransformations
/// \brief view of a subsampled version of an image_view, stepping over a number of channels in X and number of rows in Y

/// \ingroup ImageViewTransformationsSubsampled
template <typename View> 
inline typename dynamic_xy_step_type<View>::type subsampled_view(const View& src, typename View::coord_t xStep, typename View::coord_t yStep) {
    assert(xStep>0 && yStep>0);
    typedef typename dynamic_xy_step_type<View>::type RView;
    return RView((src.width()+(xStep-1))/xStep,(src.height()+(yStep-1))/yStep,
                                          typename RView::xy_locator(src.xy_at(0,0),xStep,yStep));
}

/// \ingroup ImageViewTransformationsSubsampled
template <typename View> 
inline typename dynamic_xy_step_type<View>::type subsampled_view(const View& src, const typename View::point_t& step) { 
    return subsampled_view(src,step.x,step.y);
}

/// \defgroup ImageViewTransformationsNthChannel nth_channel_view
/// \ingroup ImageViewTransformations
/// \brief single-channel (grayscale) view of the N-th channel of a given image_view

namespace detail {
    template <typename View, bool AreChannelsTogether> struct __nth_channel_view_basic;

    // nth_channel_view when the channels are not adjacent in memory. This can happen for multi-channel interleaved images 
    // or images with a step
    template <typename View>
    struct __nth_channel_view_basic<View,false> {
        typedef typename view_type<typename channel_type<View>::type, gray_layout_t, false, true, view_is_mutable<View>::value>::type type;

        static type make(const View& src, int n) {
            typedef typename type::xy_locator                             locator_t;
            typedef typename type::x_iterator                            x_iterator_t;
            typedef typename iterator_adaptor_get_base<x_iterator_t>::type x_iterator_base_t;
            x_iterator_t sit(x_iterator_base_t(&(src(0,0)[n])),src.pixels().pixel_size());
            return type(src.dimensions(),locator_t(sit, src.pixels().row_size()));
        }
    };

    // nth_channel_view when the channels are together in memory (true for simple grayscale or planar images)
    template <typename View>
    struct __nth_channel_view_basic<View,true> {
        typedef typename view_type<typename channel_type<View>::type, gray_layout_t, false, false, view_is_mutable<View>::value>::type type;
        static type make(const View& src, int n) {
            typedef typename type::x_iterator x_iterator_t;
            return interleaved_view(src.width(),src.height(),(x_iterator_t)&(src(0,0)[n]), src.pixels().row_size());
        }
    };

    template <typename View, bool IsBasic> struct __nth_channel_view;

    // For basic (memory-based) views dispatch to __nth_channel_view_basic
    template <typename View> struct __nth_channel_view<View,true> {
    private:
        typedef typename View::x_iterator src_x_iterator;

        // Determines whether the channels of a given pixel iterator are adjacent in memory.
        // Planar and grayscale iterators have channels adjacent in memory, whereas multi-channel interleaved and iterators with non-fundamental step do not.
        BOOST_STATIC_CONSTANT(bool, adjacent=
                              !iterator_is_step<src_x_iterator>::value &&
                              (is_planar<src_x_iterator>::value ||
                              num_channels<View>::value==1));
    public:
        typedef typename __nth_channel_view_basic<View,adjacent>::type type;

        static type make(const View& src, int n) {
            return __nth_channel_view_basic<View,adjacent>::make(src,n);
        }
    };

    /// \brief Function object that returns a grayscale reference of the N-th channel of a given reference. Models: PixelDereferenceAdaptorConcept.
    /// \ingroup PixelDereferenceAdaptorModel
    ///
    /// If the input is a pixel value or constant reference, the function object is immutable. Otherwise it is mutable (and returns non-const reference to the n-th channel)
    template <typename SrcP>        // SrcP is a reference to PixelConcept (could be pixel value or const/non-const reference)
                                    // Examples: pixel<T,L>, pixel<T,L>&, const pixel<T,L>&, planar_pixel_reference<T&,L>, planar_pixel_reference<const T&,L>
    struct nth_channel_deref_fn {
        BOOST_STATIC_CONSTANT(bool, is_mutable=pixel_is_reference<SrcP>::value && pixel_reference_is_mutable<SrcP>::value);
    private:
        typedef typename remove_reference<SrcP>::type src_pixel_t;
        typedef typename channel_type<src_pixel_t>::type channel_t;
        typedef typename src_pixel_t::const_reference const_ref_t;
        typedef typename pixel_reference_type<channel_t,gray_layout_t,false,is_mutable>::type ref_t;
    public:
        typedef nth_channel_deref_fn<const_ref_t>                                        const_t;
        typedef typename pixel_value_type<channel_t,gray_layout_t>::type                 value_type;
        typedef typename pixel_reference_type<channel_t,gray_layout_t,false,false>::type const_reference;
        typedef SrcP                                                                     argument_type;
        typedef typename mpl::if_c<is_mutable, ref_t, value_type>::type                  reference;
        typedef reference                                                                result_type;

        nth_channel_deref_fn(int n=0) : _n(n) {}
        template <typename P> nth_channel_deref_fn(const nth_channel_deref_fn<P>& d) : _n(d._n) {}

        int _n;        // the channel to use

        result_type operator()(argument_type srcP) const { 
            return result_type(srcP[_n]); 
        }
    };

    template <typename View> struct __nth_channel_view<View,false> {
    private:
        typedef nth_channel_deref_fn<typename View::reference> deref_t;
        typedef typename View::template add_deref<deref_t>   AD;
    public:
        typedef typename AD::type type;
        static type make(const View& src, int n) {
            return AD::make(src, deref_t(n));
        }
    };
} // namespace detail

/// \brief Given a source image view type View, returns the type of an image view over a single channel of View
/// \ingroup ImageViewTransformationsNthChannel
///
/// If the channels in the source view are adjacent in memory (such as planar non-step view or single-channel view) then the
/// return view is a single-channel non-step view.
/// If the channels are non-adjacent (interleaved and/or step view) then the return view is a single-channel step view.
template <typename View>
struct nth_channel_view_type {
private:
    GIL_CLASS_REQUIRE(View, boost::gil, ImageViewConcept)
    typedef detail::__nth_channel_view<View,view_is_basic<View>::value> VB;
public:
    typedef typename VB::type type;
    static type make(const View& src, int n) { return VB::make(src,n); }
};


/// \ingroup ImageViewTransformationsNthChannel
template <typename View>
typename nth_channel_view_type<View>::type nth_channel_view(const View& src, int n) {
    return nth_channel_view_type<View>::make(src,n);
}







/// \defgroup ImageViewTransformationsKthChannel kth_channel_view
/// \ingroup ImageViewTransformations
/// \brief single-channel (grayscale) view of the K-th channel of a given image_view. The channel index is a template parameter

namespace detail {
    template <int K, typename View, bool AreChannelsTogether> struct __kth_channel_view_basic;

    // kth_channel_view when the channels are not adjacent in memory. This can happen for multi-channel interleaved images 
    // or images with a step
    template <int K, typename View>
    struct __kth_channel_view_basic<K,View,false> {
    private:
        typedef typename kth_element_type<typename View::value_type,K>::type channel_t;
    public:
        typedef typename view_type<channel_t, gray_layout_t, false, true, view_is_mutable<View>::value>::type type;

        static type make(const View& src) {
            typedef typename type::xy_locator                             locator_t;
            typedef typename type::x_iterator                            x_iterator_t;
            typedef typename iterator_adaptor_get_base<x_iterator_t>::type x_iterator_base_t;
            x_iterator_t sit(x_iterator_base_t(&gil::at_c<K>(src(0,0))),src.pixels().pixel_size());
            return type(src.dimensions(),locator_t(sit, src.pixels().row_size()));
        }
    };

    // kth_channel_view when the channels are together in memory (true for simple grayscale or planar images)
    template <int K, typename View>
    struct __kth_channel_view_basic<K,View,true> {
    private:
        typedef typename kth_element_type<typename View::value_type, K>::type channel_t;
    public:
        typedef typename view_type<channel_t, gray_layout_t, false, false, view_is_mutable<View>::value>::type type;
        static type make(const View& src) {
            typedef typename type::x_iterator x_iterator_t;
            return interleaved_view(src.width(),src.height(),(x_iterator_t)&gil::at_c<K>(src(0,0)), src.pixels().row_size());
        }
    };

    template <int K, typename View, bool IsBasic> struct __kth_channel_view;

    // For basic (memory-based) views dispatch to __kth_channel_view_basic
    template <int K, typename View> struct __kth_channel_view<K,View,true> {
    private:
        typedef typename View::x_iterator src_x_iterator;

        // Determines whether the channels of a given pixel iterator are adjacent in memory.
        // Planar and grayscale iterators have channels adjacent in memory, whereas multi-channel interleaved and iterators with non-fundamental step do not.
        BOOST_STATIC_CONSTANT(bool, adjacent=
                              !iterator_is_step<src_x_iterator>::value &&
                              (is_planar<src_x_iterator>::value ||
                              num_channels<View>::value==1));
    public:
        typedef typename __kth_channel_view_basic<K,View,adjacent>::type type;

        static type make(const View& src) {
            return __kth_channel_view_basic<K,View,adjacent>::make(src);
        }
    };

    /// \brief Function object that returns a grayscale reference of the K-th channel (specified as a template parameter) of a given reference. Models: PixelDereferenceAdaptorConcept.
    /// \ingroup PixelDereferenceAdaptorModel
    ///
    /// If the input is a pixel value or constant reference, the function object is immutable. Otherwise it is mutable (and returns non-const reference to the k-th channel)
    template <int K, typename SrcP>        // SrcP is a reference to PixelConcept (could be pixel value or const/non-const reference)
                                    // Examples: pixel<T,L>, pixel<T,L>&, const pixel<T,L>&, planar_pixel_reference<T&,L>, planar_pixel_reference<const T&,L>
    struct kth_channel_deref_fn {
        BOOST_STATIC_CONSTANT(bool, is_mutable=pixel_is_reference<SrcP>::value && pixel_reference_is_mutable<SrcP>::value);
    private:
        typedef typename remove_reference<SrcP>::type src_pixel_t;
        typedef typename kth_element_type<src_pixel_t, K>::type channel_t;
        typedef typename src_pixel_t::const_reference const_ref_t;
        typedef typename pixel_reference_type<channel_t,gray_layout_t,false,is_mutable>::type ref_t;
    public:
        typedef kth_channel_deref_fn<K,const_ref_t>                               const_t;
        typedef typename pixel_value_type<channel_t,gray_layout_t>::type          value_type;
        typedef typename pixel_reference_type<channel_t,gray_layout_t,false,false>::type const_reference;
        typedef SrcP                                                              argument_type;
        typedef typename mpl::if_c<is_mutable, ref_t, value_type>::type           reference;
        typedef reference                                                         result_type;

        kth_channel_deref_fn() {}
        template <typename P> kth_channel_deref_fn(const kth_channel_deref_fn<K,P>&) {}

        result_type operator()(argument_type srcP) const { 
            return result_type(gil::at_c<K>(srcP));
        }
    };

    template <int K, typename View> struct __kth_channel_view<K,View,false> {
    private:
        typedef kth_channel_deref_fn<K,typename View::reference> deref_t;
        typedef typename View::template add_deref<deref_t>   AD;
    public:
        typedef typename AD::type type;
        static type make(const View& src) {
            return AD::make(src, deref_t());
        }
    };
} // namespace detail

/// \brief Given a source image view type View, returns the type of an image view over a given channel of View.
/// \ingroup ImageViewTransformationsKthChannel
///
/// If the channels in the source view are adjacent in memory (such as planar non-step view or single-channel view) then the
/// return view is a single-channel non-step view.
/// If the channels are non-adjacent (interleaved and/or step view) then the return view is a single-channel step view.
template <int K, typename View>
struct kth_channel_view_type {
private:
    GIL_CLASS_REQUIRE(View, boost::gil, ImageViewConcept)
    typedef detail::__kth_channel_view<K,View,view_is_basic<View>::value> VB;
public:
    typedef typename VB::type type;
    static type make(const View& src) { return VB::make(src); }
};

/// \ingroup ImageViewTransformationsKthChannel
template <int K, typename View>
typename kth_channel_view_type<K,View>::type kth_channel_view(const View& src) {
    return kth_channel_view_type<K,View>::make(src);
}

} }  // namespace boost::gil

#endif
