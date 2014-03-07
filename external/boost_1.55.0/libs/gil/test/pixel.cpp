/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/
// pixel.cpp : Tests GIL pixels.
//

#include <iterator>
#include <iostream>
#include <boost/type_traits.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/size.hpp>
#include <boost/gil/planar_pixel_reference.hpp>
#include <boost/gil/packed_pixel.hpp>
#include <boost/gil/rgb.hpp>
#include <boost/gil/gray.hpp>
#include <boost/gil/rgba.hpp>
#include <boost/gil/cmyk.hpp>
#include <boost/gil/pixel.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/channel_algorithm.hpp>
#include <boost/gil/color_convert.hpp>
#include <boost/gil/gil_concept.hpp>
#include <boost/gil/metafunctions.hpp>
#include <boost/gil/bit_aligned_pixel_reference.hpp>

// Testing pixel references and values, pixel operations, color conversion

using namespace boost::gil;
using std::swap;
using namespace boost;

void error_if(bool condition);

struct increment { 
    template <typename Incrementable> void operator()(Incrementable& x) const { ++x; } 
};
struct prev { 
    template <typename Subtractable> 
    typename channel_traits<Subtractable>::value_type operator()(const Subtractable& x) const { return x-1; }
};
struct set_to_one{ int operator()() const { return 1; } };

// Construct with two pixel types. They must be compatible and the second must be mutable
template <typename C1, typename C2>
struct do_basic_test : public C1, public C2 {
    typedef typename C1::type               pixel1_t;
    typedef typename C2::type               pixel2_t;
    typedef typename C1::pixel_t::value_type pixel1_value_t;
    typedef typename C2::pixel_t::value_type pixel2_value_t;
    typedef pixel1_value_t pixel_value_t;

    do_basic_test(const pixel_value_t& v) : C1(v), C2(v) {}

    void test_all() {
        test_heterogeneous();

        // test homogeneous algorithms - fill, max, min
        static const int num_chan = num_channels<typename C2::pixel_t>::value;
        static_fill(C2::_pixel, gil::at_c<0>(C1::_pixel)+1);
        error_if(gil::at_c<0>(C2::_pixel) != gil::at_c<num_chan-1>(C2::_pixel));

        C2::_pixel = C1::_pixel;
        error_if(static_max(C2::_pixel) != static_max(C1::_pixel));
        error_if(static_min(C2::_pixel) != static_min(C1::_pixel));
        error_if(static_max(C2::_pixel) < static_min(C2::_pixel));

        // test operator[]
        C2::_pixel[0] = C1::_pixel[0]+1;
        error_if(C2::_pixel[0] != C1::_pixel[0]+1);
    }

    void test_heterogeneous() {
        // Both must be pixel types (not necessarily pixel values). The second must be mutable. They must be compatible
        boost::function_requires<PixelConcept<typename C1::pixel_t> >();
        boost::function_requires<MutablePixelConcept<typename C2::pixel_t> >();
        boost::function_requires<PixelsCompatibleConcept<typename C1::pixel_t,typename C2::pixel_t> >();

        C2::_pixel = C1::_pixel;            // test operator=
        error_if(C1::_pixel != C2::_pixel);   // test operator==

        // construct a pixel value from it
        pixel1_value_t v1(C1::_pixel);
        pixel2_value_t v2(C2::_pixel);
        error_if(v1 != v2);

        // construct from a pixel value
        pixel1_t c1(v1);
        pixel2_t c2(v2);
        error_if(c1 != c2);

        // Invert the first semantic channel.
        C2::_pixel = C1::_pixel;
        semantic_at_c<0>(C2::_pixel) = channel_invert(semantic_at_c<0>(C2::_pixel));
        error_if(C1::_pixel == C2::_pixel);   // now they must not be equal

        // test pixel algorithms
        C2::_pixel = C1::_pixel;
        static_for_each(C2::_pixel, increment());
        static_transform(C2::_pixel, C2::_pixel, prev());
        error_if(C1::_pixel!=C2::_pixel);

        static_generate(C2::_pixel, set_to_one());
        error_if(gil::at_c<0>(C2::_pixel) != 1);

        // Test swap if both are mutable and if their value type is the same
        // (We know the second one is mutable)
        typedef typename boost::add_reference<typename C1::type>::type p1_ref;
        test_swap(
            boost::mpl::bool_<
                pixel_reference_is_mutable<p1_ref>::value && 
                boost::is_same<pixel1_value_t,pixel2_value_t>::value> ());
    }
     
    void test_swap(boost::mpl::false_) {}
    void test_swap(boost::mpl::true_) {
        // test swap
        static_fill(C1::_pixel, 0);
        static_fill(C2::_pixel, 1);
        pixel_value_t pv1(C1::_pixel);
        pixel_value_t pv2(C2::_pixel);
        error_if(C2::_pixel == C1::_pixel);
        swap(C1::_pixel, C2::_pixel);
        error_if(C1::_pixel != pv2 || C2::_pixel != pv1);
    }
};

template <typename PixelValue, int Tag=0>
class value_core {
public:
    typedef PixelValue type;
    typedef type        pixel_t;
    type _pixel;

    value_core() : _pixel(0) {}
    value_core(const type& val) : _pixel(val) {  // test copy constructor
        boost::function_requires<PixelValueConcept<pixel_t> >();
        type p2;            // test default constructor
    }
};

template <typename PixelRef, int Tag=0>
class reference_core : public value_core<typename boost::remove_reference<PixelRef>::type::value_type, Tag> {
public:
    typedef PixelRef type;
    typedef typename boost::remove_reference<PixelRef>::type pixel_t;
    typedef value_core<typename pixel_t::value_type, Tag> parent_t;

    type _pixel;

    reference_core() : parent_t(), _pixel(parent_t::_pixel) {}
    reference_core(const typename pixel_t::value_type& val) : parent_t(val), _pixel(parent_t::_pixel) {
        boost::function_requires<PixelConcept<pixel_t> >();
    }
};


// Use a subset of pixel models that covers all color spaces, channel depths, reference/value, planar/interleaved, const/mutable
// color conversion will be invoked on pairs of them. Having an exhaustive binary check would be too big/expensive.
typedef mpl::vector<
    value_core<gray8_pixel_t>, 
    reference_core<gray16_pixel_t&>, 
    value_core<bgr8_pixel_t>,
    reference_core<rgb8_planar_ref_t>,
    value_core<argb32_pixel_t>,
    reference_core<cmyk32f_pixel_t&>,
    reference_core<abgr16c_ref_t>,           // immutable reference
    reference_core<rgb32fc_planar_ref_t>
> representative_pixels_t;


template <typename Vector, typename Fun, int K>
struct for_each_impl {
    static void apply(Fun fun) {
        for_each_impl<Vector,Fun,K-1>::apply(fun);
        fun(typename mpl::at_c<Vector,K>::type());
    }
};

template <typename Vector, typename Fun>
struct for_each_impl<Vector,Fun,-1> {
    static void apply(Fun fun) {}
};

template <typename Vector, typename Fun>
void for_each(Fun fun) {
    for_each_impl<Vector,Fun, mpl::size<Vector>::value-1>::apply(fun);
}

template <typename Pixel1>
struct ccv2 {
    template <typename P1, typename P2>
    void color_convert_compatible(const P1& p1, P2& p2, mpl::true_) {
        typedef typename P1::value_type value_t;
        p2 = p1;
        value_t converted;
        color_convert(p1, converted);
        error_if(converted != p2);
    }

    template <typename P1, typename P2>
    void color_convert_compatible(const P1& p1, P2& p2, mpl::false_) {
        color_convert(p1,p2);
    }

    template <typename P1, typename P2>
    void color_convert_impl(const P1& p1, P2& p2) {
        color_convert_compatible(p1, p2, mpl::bool_<pixels_are_compatible<P1,P2>::value>());
    }


    template <typename Pixel2>
    void operator()(Pixel2) {
        // convert from Pixel1 to Pixel2 (or, if Pixel2 is immutable, to its value type)
        static const int p2_is_mutable = pixel_reference_is_mutable<typename Pixel2::type>::type::value;
        typedef typename boost::remove_reference<typename Pixel2::type>::type pixel_model_t;
        typedef typename pixel_model_t::value_type p2_value_t;
        typedef typename mpl::if_c<p2_is_mutable, Pixel2, value_core<p2_value_t> >::type pixel2_mutable;

        Pixel1 p1;
        pixel2_mutable p2;
        
        color_convert_impl(p1._pixel, p2._pixel);
    }
};

struct ccv1 {
    template <typename Pixel> 
    void operator()(Pixel) {
        for_each<representative_pixels_t>(ccv2<Pixel>());
    }
};

void test_color_convert() {
   for_each<representative_pixels_t>(ccv1());
}

void test_packed_pixel() {    
    typedef packed_pixel_type<uint16_t, mpl::vector3_c<unsigned,5,6,5>, rgb_layout_t>::type rgb565_pixel_t;

    boost::function_requires<PixelValueConcept<rgb565_pixel_t> >();
    BOOST_STATIC_ASSERT((sizeof(rgb565_pixel_t)==2));

    // define a bgr556 pixel
    typedef packed_pixel_type<uint16_t, mpl::vector3_c<unsigned,5,6,5>, bgr_layout_t>::type bgr556_pixel_t;
    boost::function_requires<PixelValueConcept<bgr556_pixel_t> >();

    // Create a zero packed pixel and a full regular unpacked pixel.
    rgb565_pixel_t r565;//((uint16_t)0);
    rgb8_pixel_t rgb_full(255,255,255);

    // Convert all channels of the unpacked pixel to the packed one & assert the packed one is full
    get_color(r565,red_t())   = channel_convert<kth_element_type<rgb565_pixel_t, 0>::type>(get_color(rgb_full,red_t()));
    get_color(r565,green_t()) = channel_convert<kth_element_type<rgb565_pixel_t, 1>::type>(get_color(rgb_full,green_t()));
    get_color(r565,blue_t())  = channel_convert<kth_element_type<rgb565_pixel_t, 2>::type>(get_color(rgb_full,blue_t()));
    error_if(r565 != rgb565_pixel_t((uint16_t)65535));    
    
    // rgb565 is compatible with bgr556. Test interoperability
    boost::function_requires<PixelsCompatibleConcept<rgb565_pixel_t,bgr556_pixel_t> >();

    do_basic_test<value_core<rgb565_pixel_t,0>, value_core<bgr556_pixel_t,1> >(r565).test_heterogeneous(); 

    color_convert(r565,rgb_full);
    color_convert(rgb_full,r565);

    // Test bit-aligned pixel reference
    typedef const bit_aligned_pixel_reference<boost::uint8_t, boost::mpl::vector3_c<int,1,2,1>, bgr_layout_t, true>  bgr121_ref_t;
    typedef const bit_aligned_pixel_reference<boost::uint8_t, boost::mpl::vector3_c<int,1,2,1>, rgb_layout_t, true>  rgb121_ref_t;
    typedef rgb121_ref_t::value_type rgb121_pixel_t;
    rgb121_pixel_t p121;
    do_basic_test<reference_core<bgr121_ref_t,0>, reference_core<rgb121_ref_t,1> >(p121).test_heterogeneous();     
    do_basic_test<value_core<rgb121_pixel_t,0>, reference_core<rgb121_ref_t,1> >(p121).test_heterogeneous();     

    BOOST_STATIC_ASSERT((pixel_reference_is_proxy<rgb8_planar_ref_t>::value));
    BOOST_STATIC_ASSERT((pixel_reference_is_proxy<bgr121_ref_t>::value));

    BOOST_STATIC_ASSERT(!(pixel_reference_is_proxy<rgb8_pixel_t>::value));
    BOOST_STATIC_ASSERT(!(pixel_reference_is_proxy<rgb8_pixel_t&>::value));
    BOOST_STATIC_ASSERT(!(pixel_reference_is_proxy<const rgb8_pixel_t&>::value));

    BOOST_STATIC_ASSERT( (pixel_reference_is_mutable<      rgb8_pixel_t&>::value));
    BOOST_STATIC_ASSERT(!(pixel_reference_is_mutable<const rgb8_pixel_t&>::value));

    BOOST_STATIC_ASSERT((pixel_reference_is_mutable<const rgb8_planar_ref_t&>::value));
    BOOST_STATIC_ASSERT((pixel_reference_is_mutable<      rgb8_planar_ref_t >::value));

    BOOST_STATIC_ASSERT(!(pixel_reference_is_mutable<const rgb8c_planar_ref_t&>::value));
    BOOST_STATIC_ASSERT(!(pixel_reference_is_mutable<      rgb8c_planar_ref_t >::value));

    BOOST_STATIC_ASSERT( (pixel_reference_is_mutable<bgr121_ref_t>::value));
    BOOST_STATIC_ASSERT(!(pixel_reference_is_mutable<bgr121_ref_t::const_reference>::value));

}

void test_pixel() {
    test_packed_pixel();
    rgb8_pixel_t rgb8(1,2,3);

    do_basic_test<value_core<rgb8_pixel_t,0>, reference_core<rgb8_pixel_t&,1> >(rgb8).test_all(); 
    do_basic_test<value_core<bgr8_pixel_t,0>, reference_core<rgb8_planar_ref_t,1> >(rgb8).test_all(); 
    do_basic_test<reference_core<rgb8_planar_ref_t,0>, reference_core<bgr8_pixel_t&,1> >(rgb8).test_all(); 
    do_basic_test<reference_core<const rgb8_pixel_t&,0>, reference_core<rgb8_pixel_t&,1> >(rgb8).test_all(); 

    test_color_convert();

    // Semantic vs physical channel accessors. Named channel accessors
    bgr8_pixel_t bgr8(rgb8);
    error_if(bgr8[0] == rgb8[0]);
    error_if(dynamic_at_c(bgr8,0) == dynamic_at_c(rgb8,0));
    error_if(gil::at_c<0>(bgr8) == gil::at_c<0>(rgb8));
    error_if(semantic_at_c<0>(bgr8) != semantic_at_c<0>(rgb8));
    error_if(get_color(bgr8,blue_t()) != get_color(rgb8,blue_t()));

    // Assigning a grayscale channel to a pixel
    gray16_pixel_t g16(34);
    g16 = 8;
    bits16 g = get_color(g16,gray_color_t());
    error_if(g != 8);
    error_if(g16 != 8);
}

int main(int argc, char* argv[]) {
    test_pixel();
    return 0;
}

