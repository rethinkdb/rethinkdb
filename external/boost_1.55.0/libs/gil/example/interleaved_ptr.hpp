/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/

/*************************************************************************************************/

////////////////////////////////////////////////////////////////////////////////////////
/// \file               
/// \brief Example on how to create a pixel iterator
/// \author Lubomir Bourdev and Hailin Jin \n
///         Adobe Systems Incorporated
/// \date 2005-2007 \n Last updated on February 26, 2007
///
/// Definitions of standard GIL channel models
///
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GIL_INTERLEAVED_PTR_HPP
#define GIL_INTERLEAVED_PTR_HPP

#include <boost/gil/pixel_iterator.hpp>
#include "interleaved_ref.hpp"

namespace boost { namespace gil {

/////////////////////////////////////////////////////////////////////////
///
/// A model of an interleaved pixel iterator. Contains an iterator to the first channel of the current pixel
///
/// Models: 
///     MutablePixelIteratorConcept
///        PixelIteratorConcept
///           boost_concepts::RandomAccessTraversalConcept
///           PixelBasedConcept
///     HomogeneousPixelBasedConcept
///        PixelBasedConcept
///     ByteAdvanceableConcept
///     HasDynamicXStepTypeConcept
///
/////////////////////////////////////////////////////////////////////////

template <typename ChannelPtr,  // Models Channel Iterator (examples: unsigned char* or const unsigned char*)
          typename Layout>      // A layout (includes the color space and channel ordering)
struct interleaved_ptr : public  boost::iterator_facade<interleaved_ptr<ChannelPtr,Layout>,
                                   pixel<typename std::iterator_traits<ChannelPtr>::value_type,Layout>,
                                   boost::random_access_traversal_tag,
                                   const interleaved_ref<typename std::iterator_traits<ChannelPtr>::reference,Layout> >
{
private:
    typedef boost::iterator_facade<interleaved_ptr<ChannelPtr,Layout>,
                                   pixel<typename std::iterator_traits<ChannelPtr>::value_type,Layout>,
                                   boost::random_access_traversal_tag,
                                   const interleaved_ref<typename std::iterator_traits<ChannelPtr>::reference,Layout> > parent_t;
    typedef typename std::iterator_traits<ChannelPtr>::value_type channel_t;
public:
    typedef typename parent_t::reference                  reference;
    typedef typename parent_t::difference_type            difference_type;

    interleaved_ptr() {}
    interleaved_ptr(const interleaved_ptr& ptr) : _channels(ptr._channels) {}
    template <typename CP> interleaved_ptr(const interleaved_ptr<CP,Layout>& ptr) : _channels(ptr._channels) {}

    interleaved_ptr(const ChannelPtr& channels) : _channels(channels) {}

    // Construct from a pointer to the reference type. Not required by concepts but important
    interleaved_ptr(reference* pix) : _channels(&((*pix)[0])) {}
    interleaved_ptr& operator=(reference* pix) { _channels=&((*pix)[0]); return *this; }

    /// For some reason operator[] provided by boost::iterator_facade returns a custom class that is convertible to reference
    /// We require our own reference because it is registered in iterator_traits
    reference operator[](difference_type d) const { return memunit_advanced_ref(*this,d*sizeof(channel_t));}

    // Put this for every iterator whose reference is a proxy type
    reference operator->()                  const { return **this; }

    // Channels accessor (not required by any concept)
    const ChannelPtr& channels()            const { return _channels; }
          ChannelPtr& channels()                  { return _channels; }
    
    // Not required by concepts but useful
    static const std::size_t num_channels = mpl::size<typename Layout::color_space_t>::value;
private:
    ChannelPtr _channels;
    friend class boost::iterator_core_access;
    template <typename CP, typename L> friend struct interleaved_ptr;

    void increment()            { _channels+=num_channels; }
    void decrement()            { _channels-=num_channels; }
    void advance(ptrdiff_t d)   { _channels+=num_channels*d; }

    ptrdiff_t distance_to(const interleaved_ptr& it) const { return (it._channels-_channels)/num_channels; }
    bool equal(const interleaved_ptr& it) const { return _channels==it._channels; }

    reference dereference() const { return reference(_channels); }
};

/////////////////////////////
//  PixelIteratorConcept
/////////////////////////////

// To get from the channel pointer a channel pointer to const, we have to go through the channel traits, which take a model of channel
// So we can get a model of channel from the channel pointer via iterator_traits. Notice that we take the iterator_traits::reference and not
// iterator_traits::value_type. This is because sometimes multiple reference and pointer types share the same value type. An example of this is
// GIL's planar reference and iterator ("planar_pixel_reference" and "planar_pixel_iterator") which share the class "pixel" as the value_type. The
// class "pixel" is also the value type for interleaved pixel references. Here we are dealing with channels, not pixels, but the principles still apply.
template <typename ChannelPtr, typename Layout>
struct const_iterator_type<interleaved_ptr<ChannelPtr,Layout> > { 
private:
    typedef typename std::iterator_traits<ChannelPtr>::reference channel_ref_t;
    typedef typename channel_traits<channel_ref_t>::const_pointer channel_const_ptr_t;
public:
    typedef interleaved_ptr<channel_const_ptr_t,Layout> type; 
};

template <typename ChannelPtr, typename Layout>
struct iterator_is_mutable<interleaved_ptr<ChannelPtr,Layout> > : public boost::mpl::true_ {};
template <typename Channel, typename Layout>
struct iterator_is_mutable<interleaved_ptr<const Channel*,Layout> > : public boost::mpl::false_ {};

template <typename ChannelPtr, typename Layout>
struct is_iterator_adaptor<interleaved_ptr<ChannelPtr,Layout> > : public boost::mpl::false_ {};

/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template <typename ChannelPtr, typename Layout>
struct color_space_type<interleaved_ptr<ChannelPtr,Layout> > {
    typedef typename Layout::color_space_t type;
};

template <typename ChannelPtr, typename Layout>
struct channel_mapping_type<interleaved_ptr<ChannelPtr,Layout> > {
    typedef typename Layout::channel_mapping_t type;
};

template <typename ChannelPtr, typename Layout>
struct is_planar<interleaved_ptr<ChannelPtr,Layout> > : public mpl::false_ {};

/////////////////////////////
//  HomogeneousPixelBasedConcept
/////////////////////////////

template <typename ChannelPtr, typename Layout>
struct channel_type<interleaved_ptr<ChannelPtr,Layout> > {
    typedef typename std::iterator_traits<ChannelPtr>::value_type type;
};

/////////////////////////////
//  ByteAdvanceableConcept
/////////////////////////////

template <typename ChannelPtr, typename Layout>
inline std::ptrdiff_t memunit_step(const interleaved_ptr<ChannelPtr,Layout>&) { 
    return sizeof(typename std::iterator_traits<ChannelPtr>::value_type)*   // size of each channel in bytes
           interleaved_ptr<ChannelPtr,Layout>::num_channels;                // times the number of channels
}

template <typename ChannelPtr, typename Layout>
inline std::ptrdiff_t memunit_distance(const interleaved_ptr<ChannelPtr,Layout>& p1, const interleaved_ptr<ChannelPtr,Layout>& p2) { 
    return memunit_distance(p1.channels(),p2.channels()); 
}

template <typename ChannelPtr, typename Layout>
inline void memunit_advance(interleaved_ptr<ChannelPtr,Layout>& p, std::ptrdiff_t diff) { 
    memunit_advance(p.channels(), diff);
}

template <typename ChannelPtr, typename Layout>
inline interleaved_ptr<ChannelPtr,Layout> memunit_advanced(const interleaved_ptr<ChannelPtr,Layout>& p, std::ptrdiff_t diff) {
    interleaved_ptr<ChannelPtr,Layout> ret=p;
    memunit_advance(ret, diff);
    return ret;
}

template <typename ChannelPtr, typename Layout>
inline typename interleaved_ptr<ChannelPtr,Layout>::reference memunit_advanced_ref(const interleaved_ptr<ChannelPtr,Layout>& p, std::ptrdiff_t diff) {
    interleaved_ptr<ChannelPtr,Layout> ret=p;
    memunit_advance(ret, diff);
    return *ret;
}

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template <typename ChannelPtr, typename Layout>
struct dynamic_x_step_type<interleaved_ptr<ChannelPtr,Layout> > {
    typedef memory_based_step_iterator<interleaved_ptr<ChannelPtr,Layout> > type;
};

} }  // namespace boost::gil

#endif
