/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://stlab.adobe.com/gil for most recent version including documentation.
*/

/*************************************************************************************************/


////////////////////////////////////////////////////////////////////////////////////////
/// \file               
/// \brief Example on how to create a new model of a pixel reference
/// \author Lubomir Bourdev and Hailin Jin \n
///         Adobe Systems Incorporated
/// \date 2005-2007 \n Last updated on February 26, 2007
//////
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GIL_INTERLEAVED_REF_HPP
#define GIL_INTERLEAVED_REF_HPP

#include <boost/mpl/range_c.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/gil/extension/dynamic_image/dynamic_image_all.hpp>

namespace boost { namespace gil {


/////////////////////////////////////////////////////////////////////////
///
/// A model of an interleaved pixel reference. Holds a pointer to the first channel
/// MODELS: 
///    MutableHomogeneousPixelConcept
///       MutableHomogeneousColorBaseConcept
///           MutableColorBaseConcept 
///           HomogeneousColorBaseConcept
///               ColorBaseConcept
///    HomogeneousPixelBasedConcept
///       PixelBasedConcept
///
/// For planar reference proxies to work properly, all of their methods must be const-qualified
/// and their iterator's reference type must be const-qualified. 
/// Mutability of the reference proxy is part of its type (in this case, depends on the mutability of ChannelReference)
/////////////////////////////////////////////////////////////////////////

template <typename ChannelReference, // Models ChannelConcept. A channel reference, unsigned char& or const unsigned char& 
          typename Layout>           // A layout (includes the color space and channel ordering)
struct interleaved_ref {
private:
    typedef typename channel_traits<ChannelReference>::value_type      channel_t;
    typedef typename channel_traits<ChannelReference>::reference       channel_reference_t;
    typedef typename channel_traits<ChannelReference>::const_reference channel_const_reference_t;
    typedef typename channel_traits<ChannelReference>::pointer         channel_pointer_t;
public:
// Required by ColorBaseConcept
    typedef Layout layout_t;

    // Copy construction from a compatible type. The copy constructor of references is shallow. The channels themselves are not copied.
    interleaved_ref(const interleaved_ref& p) : _channels(p._channels) {}
    template <typename P> interleaved_ref(const P& p) : _channels(p._channels) { check_compatible<P>(); }

    template <typename P> bool operator==(const P& p)    const { check_compatible<P>(); return static_equal(*this,p); }
    template <typename P> bool operator!=(const P& p)    const { return !(*this==p); }

// Required by MutableColorBaseConcept
    
    // Assignment from a compatible type 
    const interleaved_ref&  operator=(const interleaved_ref& p)  const { static_copy(p,*this); return *this; }
    template <typename P> const interleaved_ref& operator=(const P& p) const { check_compatible<P>(); static_copy(p,*this); return *this; }

// Required by PixelConcept
    typedef pixel<channel_t, layout_t> value_type;
    typedef interleaved_ref            reference;
    typedef interleaved_ref<channel_const_reference_t, layout_t> const_reference;
    static const bool is_mutable = channel_traits<ChannelReference>::is_mutable;

// Required by HomogeneousPixelConcept
    ChannelReference                   operator[](std::size_t i) const { return _channels[i]; }

// Custom constructor (not part of any concept)
    explicit interleaved_ref(channel_pointer_t channels) : _channels(channels) {}
// This is needed for the reference proxy to work properly
    const interleaved_ref*             operator->()              const { return this; }
private:
    channel_pointer_t _channels;

    template <typename Pixel> static void check_compatible() { gil_function_requires<PixelsCompatibleConcept<Pixel,interleaved_ref> >(); }
};

// Required by ColorBaseConcept
template <typename ChannelReference, typename Layout, int K>
struct kth_element_type<interleaved_ref<ChannelReference,Layout>,K> { 
    typedef ChannelReference type; 
};

template <typename ChannelReference, typename Layout, int K>
struct kth_element_reference_type<interleaved_ref<ChannelReference,Layout>,K> { 
    typedef ChannelReference type; 
};

template <typename ChannelReference, typename Layout, int K>
struct kth_element_const_reference_type<interleaved_ref<ChannelReference,Layout>,K> {
    typedef ChannelReference type; 
//    typedef typename channel_traits<ChannelReference>::const_reference type; 
};


// Required by ColorBaseConcept
template <int K, typename ChannelReference, typename Layout>
typename element_reference_type<interleaved_ref<ChannelReference,Layout> >::type
at_c(const interleaved_ref<ChannelReference,Layout>& p) { return p[K]; };

// Required by HomogeneousColorBaseConcept
template <typename ChannelReference, typename Layout>
typename element_reference_type<interleaved_ref<ChannelReference,Layout> >::type
dynamic_at_c(const interleaved_ref<ChannelReference,Layout>& p, std::size_t n) { return p[n]; };

namespace detail {
    struct swap_fn_t {
        template <typename T> void operator()(T& x, T& y) const {
            using std::swap;
            swap(x,y);
        }
    };
}

// Required by MutableColorBaseConcept. The default std::swap does not do the right thing for proxy references - it swaps the references, not the values
template <typename ChannelReference, typename Layout>
void swap(const interleaved_ref<ChannelReference,Layout>& x, const interleaved_ref<ChannelReference,Layout>& y) { 
    static_for_each(x,y,detail::swap_fn_t());
};

// Required by PixelConcept
template <typename ChannelReference, typename Layout>
struct is_pixel<interleaved_ref<ChannelReference,Layout> > : public boost::mpl::true_ {};


// Required by PixelBasedConcept
template <typename ChannelReference, typename Layout>
struct color_space_type<interleaved_ref<ChannelReference,Layout> > {
    typedef typename Layout::color_space_t type;
}; 

// Required by PixelBasedConcept
template <typename ChannelReference, typename Layout>
struct channel_mapping_type<interleaved_ref<ChannelReference,Layout> > {
    typedef typename Layout::channel_mapping_t type;
}; 

// Required by PixelBasedConcept
template <typename ChannelReference, typename Layout>
struct is_planar<interleaved_ref<ChannelReference,Layout> > : mpl::false_ {};

// Required by HomogeneousPixelBasedConcept
template <typename ChannelReference, typename Layout>
struct channel_type<interleaved_ref<ChannelReference,Layout> > {
    typedef typename channel_traits<ChannelReference>::value_type type;
}; 

} }  // namespace boost::gil

#endif
