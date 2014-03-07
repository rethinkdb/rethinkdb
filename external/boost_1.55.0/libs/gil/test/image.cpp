/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/
// image_test.cpp : 
//

#ifdef _MSC_VER
//#pragma warning(disable : 4244)     // conversion from 'gil::image<V,Alloc>::coord_t' to 'int', possible loss of data (visual studio compiler doesn't realize that the two types are the same)
#pragma warning(disable : 4503)     // decorated name length exceeded, name was truncated
#endif

#include <string>
#include <vector>
#include <ios>
#include <iostream>
#include <fstream>
#include <map>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/gil/extension/dynamic_image/dynamic_image_all.hpp>
#include <boost/crc.hpp>

using namespace boost::gil;
using namespace std;
using namespace boost;

extern rgb8c_planar_view_t sample_view;
void error_if(bool condition);


// When BOOST_GIL_GENERATE_REFERENCE_DATA is defined, the reference data is generated and saved.
// When it is undefined, regression tests are checked against it
//#define BOOST_GIL_GENERATE_REFERENCE_DATA

////////////////////////////////////////////////////
///
///  Some algorithms to use in testing
///
////////////////////////////////////////////////////

template <typename GrayView, typename R>
void gray_image_hist(const GrayView& img_view, R& hist) {
//    for_each_pixel(img_view,++lambda::var(hist)[lambda::_1]);
    for (typename GrayView::iterator it=img_view.begin(); it!=img_view.end(); ++it)
        ++hist[*it];
}

template <typename V, typename R>
void get_hist(const V& img_view, R& hist) {
    gray_image_hist(color_converted_view<gray8_pixel_t>(img_view), hist);
}

// testing custom color conversion
template <typename C1, typename C2>
struct my_color_converter_impl : public default_color_converter_impl<C1,C2> {};
template <typename C1>
struct my_color_converter_impl<C1,gray_t> {
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const {
        default_color_converter_impl<C1,gray_t>()(src,dst);
        get_color(dst,gray_color_t())=channel_invert(get_color(dst,gray_color_t()));
    }
};

struct my_color_converter {
    template <typename SrcP,typename DstP>
    void operator()(const SrcP& src,DstP& dst) const { 
        typedef typename color_space_type<SrcP>::type src_cs_t;
        typedef typename color_space_type<DstP>::type dst_cs_t;
        my_color_converter_impl<src_cs_t,dst_cs_t>()(src,dst);
    }
};

// Models a Unary Function
template <typename P>   // Models PixelValueConcept
struct mandelbrot_fn {
    typedef point2<std::ptrdiff_t>    point_t;

    typedef mandelbrot_fn        const_t;
    typedef P                    value_type;
    typedef value_type           reference;
    typedef value_type           const_reference;
    typedef point_t              argument_type;
    typedef reference            result_type;
    BOOST_STATIC_CONSTANT(bool, is_mutable=false);

    value_type                    _in_color,_out_color;
    point_t                       _img_size;
    static const int MAX_ITER=100;        // max number of iterations

    mandelbrot_fn() {}
    mandelbrot_fn(const point_t& sz, const value_type& in_color, const value_type& out_color) : _in_color(in_color), _out_color(out_color), _img_size(sz) {}

    result_type operator()(const point_t& p) const {
        // normalize the coords to (-2..1, -1.5..1.5)
        // (actually make y -1.0..2 so it is asymmetric, so we can verify some view factory methods)
        double t=get_num_iter(point2<double>(p.x/(double)_img_size.x*3-2, p.y/(double)_img_size.y*3-1.0f));//1.5f));
        t=pow(t,0.2);

        value_type ret;
        for (int k=0; k<num_channels<P>::value; ++k)
            ret[k]=(typename channel_type<value_type>::type)(_in_color[k]*t + _out_color[k]*(1-t));
        return ret;
    }

private:
    double get_num_iter(const point2<double>& p) const {
        point2<double> Z(0,0);
        for (int i=0; i<MAX_ITER; ++i) {
            Z = point2<double>(Z.x*Z.x - Z.y*Z.y + p.x, 2*Z.x*Z.y + p.y);
            if (Z.x*Z.x + Z.y*Z.y > 4)
                return i/(double)MAX_ITER;
        }
        return 0;
    }
};

template <typename T>
void x_gradient(const T& src, const gray8s_view_t& dst) {
    for (int y=0; y<src.height(); ++y) {
        typename T::x_iterator src_it = src.row_begin(y);
        gray8s_view_t::x_iterator dst_it = dst.row_begin(y);

        for (int x=1; x<src.width()-1; ++x)
            dst_it[x] = (src_it[x+1] - src_it[x-1]) / 2;
    }
}

// A quick test whether a view is homogeneous

template <typename Pixel>
struct pixel_is_homogeneous : public mpl::true_ {};

template <typename P, typename C, typename L>
struct pixel_is_homogeneous<packed_pixel<P,C,L> > : public mpl::false_ {};

template <typename View>
struct view_is_homogeneous : public pixel_is_homogeneous<typename View::value_type> {};


////////////////////////////////////////////////////
///
///  Tests image view transformations and algorithms
///
////////////////////////////////////////////////////
class image_test {
public:
    virtual void initialize() {}
    virtual void finalize() {}
    virtual ~image_test() {}

    void run();
protected:
    virtual void check_view_impl(const rgb8c_view_t& view, const string& name)=0;
    template <typename View>
    void check_view(const View& img_view, const string& name) {
        rgb8_image_t rgb_img(img_view.dimensions());
        copy_and_convert_pixels(img_view,view(rgb_img));
        check_view_impl(const_view(rgb_img), name);
    }
private:
    template <typename Img> void basic_test(const string& prefix);
    template <typename View> void view_transformations_test(const View& img_view, const string& prefix);
    template <typename View> void homogeneous_view_transformations_test(const View& img_view, const string& prefix, mpl::true_);
    template <typename View> void homogeneous_view_transformations_test(const View& img_view, const string& prefix, mpl::false_) {}
    template <typename View> void histogram_test(const View& img_view, const string& prefix);
    void virtual_view_test();
    void packed_image_test();
    void dynamic_image_test();
    template <typename Img> void image_all_test(const string& prefix);
};


// testing image iterators, clone, fill, locators, color convert
template <typename Img>
void image_test::basic_test(const string& prefix) {
    typedef typename Img::view_t View;

    // make a 20x20 image
    Img img(typename View::point_t(20,20));
    const View& img_view=view(img);

    // fill it with red
    rgb8_pixel_t red8(255,0,0), green8(0,255,0), blue8(0,0,255), white8(255,255,255);
    typename View::value_type red,green,blue,white;
    color_convert(red8,red);
    default_color_converter()(red8,red);
    red=color_convert_deref_fn<rgb8_ref_t,typename Img::view_t::value_type>()(red8);

    color_convert(green8,green);
    color_convert(blue8,blue);
    color_convert(white8,white);
    fill(img_view.begin(),img_view.end(),red);

    color_convert(red8,img_view[0]);

    // pointer to first pixel of second row
    typename View::reference rt=img_view.at(0,0)[img_view.width()];
    typename View::x_iterator ptr=&rt;
    typename View::reference rt2=*(img_view.at(0,0)+img_view.width());
    typename View::x_iterator ptr2=&rt2;
    error_if(ptr!=ptr2);
    error_if(img_view.x_at(0,0)+10!=10+img_view.x_at(0,0));

    // draw a blue line along the diagonal
    typename View::xy_locator loc=img_view.xy_at(0,img_view.height()-1);
    for (int y=0; y<img_view.height(); ++y) {
        *loc=blue;
        ++loc.x();
        loc.y()--;
    }

    // draw a green dotted line along the main diagonal with step of 3
    loc=img_view.xy_at(img_view.width()-1,img_view.height()-1);
    while (loc.x()>=img_view.x_at(0,0)) {
        *loc=green;
        loc-=typename View::point_t(3,3);
    }

    // Clone and make every red pixel white
    Img imgWhite(img);
    for (typename View::iterator it=view(imgWhite).end(); (it-1)!=view(imgWhite).begin(); --it) {
        if (*(it-1)==red)
            *(it-1)=white;
    }

    check_view(img_view,prefix+"red_x");
    check_view(view(imgWhite),prefix+"white_x");
}

template <typename View>
void image_test::histogram_test(const View& img_view, const string& prefix) {
//  vector<int> histogram(255,0);
//  get_hist(cropped,histogram.begin());
    unsigned char histogram[256];
    fill(histogram,histogram+256,0);
    get_hist(img_view,histogram);
    gray8c_view_t hist_view=interleaved_view(256,1,(const gray8_pixel_t*)histogram,256);
    check_view(hist_view,prefix+"histogram");
}


template <typename View>
void image_test::view_transformations_test(const View& img_view, const string& prefix) {
    check_view(img_view,prefix+"original");

    check_view(subimage_view(img_view, iround(img_view.dimensions()/4), iround(img_view.dimensions()/2)),prefix+"cropped");
    check_view(color_converted_view<gray8_pixel_t>(img_view),prefix+"gray8");
    check_view(color_converted_view<gray8_pixel_t>(img_view,my_color_converter()),prefix+"my_gray8");
    check_view(transposed_view(img_view),prefix+"transpose");
    check_view(rotated180_view(img_view),prefix+"rot180");
    check_view(rotated90cw_view(img_view),prefix+"90cw");
    check_view(rotated90ccw_view(img_view),prefix+"90ccw");
    check_view(flipped_up_down_view(img_view),prefix+"flipped_ud");
    check_view(flipped_left_right_view(img_view),prefix+"flipped_lr");
    check_view(subsampled_view(img_view,typename View::point_t(2,1)),prefix+"subsampled");   
    check_view(kth_channel_view<0>(img_view),prefix+"0th_k_channel");
    homogeneous_view_transformations_test(img_view, prefix, view_is_homogeneous<View>());
}

template <typename View>
void image_test::homogeneous_view_transformations_test(const View& img_view, const string& prefix, mpl::true_) {
    check_view(nth_channel_view(img_view,0),prefix+"0th_n_channel");
}


void image_test::virtual_view_test() {
    typedef mandelbrot_fn<rgb8_pixel_t> deref_t;
    typedef deref_t::point_t            point_t;
    typedef virtual_2d_locator<deref_t,false> locator_t;
    typedef image_view<locator_t> my_virt_view_t;

    boost::function_requires<PixelLocatorConcept<locator_t> >();
    gil_function_requires<StepIteratorConcept<locator_t::x_iterator> >();

    point_t dims(200,200);
    my_virt_view_t mandel(dims, locator_t(point_t(0,0), point_t(1,1), deref_t(dims, rgb8_pixel_t(255,0,255), rgb8_pixel_t(0,255,0))));

    gray8s_image_t img(dims);
    fill_pixels(view(img),0);   // our x_gradient algorithm doesn't change the first & last column, so make sure they are 0
    x_gradient(color_converted_view<gray8_pixel_t>(mandel), view(img));
    check_view(color_converted_view<gray8_pixel_t>(const_view(img)), "mandelLuminosityGradient");

    view_transformations_test(mandel,"virtual_");
    histogram_test(mandel,"virtual_");
}

// Test alignment and packed images
void image_test::packed_image_test() {
    typedef bit_aligned_image3_type<1,3,1, bgr_layout_t>::type bgr131_image_t;
    typedef bgr131_image_t::value_type bgr131_pixel_t;
    bgr131_pixel_t fill_val(1,3,1);

    bgr131_image_t bgr131_img(3,10);
    fill_pixels(view(bgr131_img), fill_val);

    bgr131_image_t bgr131a_img(3,10,1);
    copy_pixels(const_view(bgr131_img), view(bgr131a_img));

    bgr131_image_t bgr131b_img(3,10,4);
    copy_pixels(const_view(bgr131_img), view(bgr131b_img));

    error_if(bgr131_img!=bgr131a_img || bgr131a_img!=bgr131b_img);
}

void image_test::dynamic_image_test() {
    typedef any_image<mpl::vector<gray8_image_t, bgr8_image_t, argb8_image_t,
                                  rgb8_image_t, rgb8_planar_image_t> > any_image_t;
    rgb8_planar_image_t img(sample_view.dimensions());
    copy_pixels(sample_view, view(img));
    any_image_t any_img=any_image_t(img);

    check_view(view(any_img), "dynamic_");
    check_view(flipped_left_right_view(view(any_img)), "dynamic_fliplr");
    check_view(flipped_up_down_view(view(any_img)), "dynamic_flipud");
    
    any_image_t::view_t subimageView=subimage_view(view(any_img),0,0,10,15);

    check_view(subimageView, "dynamic_subimage");
    check_view(subsampled_view(rotated180_view(view(any_img)), 2,1), "dynamic_subimage_subsampled180rot");
}

template <typename Img>
void image_test::image_all_test(const string& prefix) {
    basic_test<Img>(prefix+"basic_");

    Img img;
    img.recreate(sample_view.dimensions());
    copy_and_convert_pixels(sample_view,view(img));

    view_transformations_test(view(img), prefix+"views_");

    histogram_test(const_view(img),prefix+"histogram_");
}

void image_test::run() {
    initialize();

    image_all_test<bgr8_image_t>("bgr8_");
    image_all_test<rgb8_image_t>("rgb8_");
    image_all_test<rgb8_planar_image_t>("planarrgb8_");
    image_all_test<gray8_image_t>("gray8_");

    typedef const bit_aligned_pixel_reference<boost::uint8_t, mpl::vector3_c<int,1,2,1>, bgr_layout_t, true>  bgr121_ref_t;
    typedef image<bgr121_ref_t,false> bgr121_image_t;
    image_all_test<bgr121_image_t>("bgr121_");

    // TODO: Remove?
    view_transformations_test(subsampled_view(sample_view,point2<ptrdiff_t>(1,2)),"subsampled_");
    view_transformations_test(color_converted_view<gray8_pixel_t>(sample_view),"color_converted_");

    virtual_view_test();
    packed_image_test();
    dynamic_image_test();

    finalize();
}



////////////////////////////////////////////////////
///
///  Performs or generates image tests using checksums
///
////////////////////////////////////////////////////

class checksum_image_mgr : public image_test {
protected:
    typedef map<string,boost::crc_32_type::value_type> crc_map_t;
    crc_map_t _crc_map;
};

////////////////////////////////////////////////////
///
///  Performs image tests by comparing image pixel checksums against a reference
///
////////////////////////////////////////////////////

class checksum_image_test : public checksum_image_mgr {
public:
    checksum_image_test(const char* filename) : _filename(filename) {}
private:
    const char* _filename;
    virtual void initialize();
    virtual void check_view_impl(const rgb8c_view_t& v, const string& name);
};

// Load the checksums from the reference file and create the start image
void checksum_image_test::initialize() {
    string crc_name; 
    boost::crc_32_type::value_type crc_result;
    fstream checksum_ref(_filename,ios::in);
    while (true) {
        checksum_ref >> crc_name >> std::hex >> crc_result;
        if(checksum_ref.fail()) break;
        _crc_map[crc_name]=crc_result;
    }
    checksum_ref.close();
}

// Create a checksum for the given view and compare it with the reference checksum. Throw exception if different
void checksum_image_test::check_view_impl(const rgb8c_view_t& img_view, const string& name) {
    boost::crc_32_type checksum_acumulator;
    checksum_acumulator.process_bytes(img_view.row_begin(0),img_view.size()*3);

    cerr << "Checking checksum for " << name << endl;
    if (checksum_acumulator.checksum()!=_crc_map[name]) {
        cerr << "Checksum error in "<<name<<"\n";
        error_if(true);
    }
}

////////////////////////////////////////////////////
///
///  Generates a set of reference checksums to compare against
///
////////////////////////////////////////////////////

class checksum_image_generate : public checksum_image_mgr {
public:
    checksum_image_generate(const char* filename) : _filename(filename) {}
private:
    const char* _filename;
    virtual void check_view_impl(const rgb8c_view_t& img_view, const string& name);
    virtual void finalize();
};

// Add the checksum of the given view to the map of checksums
void checksum_image_generate::check_view_impl(const rgb8c_view_t& img_view, const string& name) {
    boost::crc_32_type result;
    result.process_bytes(img_view.row_begin(0),img_view.size()*3);
    cerr << "Generating checksum for " << name << endl;
    _crc_map[name] = result.checksum();
}

// Save the checksums into the reference file
void checksum_image_generate::finalize() {
    fstream checksum_ref(_filename,ios::out);
    for (crc_map_t::const_iterator it=_crc_map.begin(); it!=_crc_map.end(); ++it) {
        checksum_ref << it->first << " " << std::hex << it->second << "\r\n";
    }
    checksum_ref.close();
}


////////////////////////////////////////////////////
///
///  Performs or generates image tests using image I/O
///
////////////////////////////////////////////////////

extern const string in_dir;
extern const string out_dir;
extern const string ref_dir;

const string in_dir="";  // directory of source images
const string out_dir=in_dir+"image-out/";    // directory where to write output
const string ref_dir=in_dir+"image-ref/";  // reference directory to compare written with actual output

#ifndef BOOST_GIL_NO_IO

#include <boost/gil/extension/io/jpeg_io.hpp>

class file_image_mgr : public image_test {};

class file_image_test : public file_image_mgr {
public:
    file_image_test(const char*) {}
protected:
    void check_view_impl(const boost::gil::rgb8c_view_t& img_view,const string& name) {
        jpeg_write_view(out_dir+name+".jpg",img_view);
        rgb8_image_t img1, img2;
        jpeg_read_and_convert_image(out_dir+name+".jpg",img1);
        cerr << "Testing "<<name<<"\n";

        jpeg_read_and_convert_image(ref_dir+name+".jpg",img2);
        if (img1!=img2) {
            cerr << "Error with "<<name<<"\n";
            error_if(true);
        }
    }
};

class file_image_generate : public file_image_mgr {
public:
    file_image_generate(const char*) {}
protected:
    void check_view_impl(const boost::gil::rgb8c_view_t& img_view,const string& name) {
        jpeg_write_view(ref_dir+name+".jpg",img_view);
        cerr << "Writing "<<name<<"\n";
    }
};
#endif








void static_checks() {
    gil_function_requires<ImageConcept<rgb8_image_t> >();

    BOOST_STATIC_ASSERT(view_is_basic<rgb8_step_view_t>::value);
    BOOST_STATIC_ASSERT(view_is_basic<cmyk8c_planar_step_view_t>::value);
    BOOST_STATIC_ASSERT(view_is_basic<rgb8_planar_view_t>::value);

    BOOST_STATIC_ASSERT(view_is_step_in_x<rgb8_step_view_t>::value);
    BOOST_STATIC_ASSERT(view_is_step_in_x<cmyk8c_planar_step_view_t>::value);
    BOOST_STATIC_ASSERT(!view_is_step_in_x<rgb8_planar_view_t>::value);

    BOOST_STATIC_ASSERT(!is_planar<rgb8_step_view_t>::value);
    BOOST_STATIC_ASSERT(is_planar<cmyk8c_planar_step_view_t>::value);
    BOOST_STATIC_ASSERT(is_planar<rgb8_planar_view_t>::value);

    BOOST_STATIC_ASSERT(view_is_mutable<rgb8_step_view_t>::value);
    BOOST_STATIC_ASSERT(!view_is_mutable<cmyk8c_planar_step_view_t>::value);
    BOOST_STATIC_ASSERT(view_is_mutable<rgb8_planar_view_t>::value);

    BOOST_STATIC_ASSERT((boost::is_same<derived_view_type<cmyk8c_planar_step_view_t>::type, cmyk8c_planar_step_view_t>::value));
    BOOST_STATIC_ASSERT((boost::is_same<derived_view_type<cmyk8c_planar_step_view_t, bits16, rgb_layout_t>::type,  rgb16c_planar_step_view_t>::value));
    BOOST_STATIC_ASSERT((boost::is_same<derived_view_type<cmyk8c_planar_step_view_t, use_default, rgb_layout_t, mpl::false_, use_default, mpl::false_>::type,  rgb8c_step_view_t>::value));

    // test view get raw data (mostly compile-time test)
    {
    rgb8_image_t rgb8(100,100);
    unsigned char* data=interleaved_view_get_raw_data(view(rgb8));
    const unsigned char* cdata=interleaved_view_get_raw_data(const_view(rgb8));
    error_if(data!=cdata);
    }

    {
    rgb16s_planar_image_t rgb8(100,100);
    short* data=planar_view_get_raw_data(view(rgb8),1);
    const short* cdata=planar_view_get_raw_data(const_view(rgb8),1);
    error_if(data!=cdata);
    }
}

#ifdef BOOST_GIL_NO_IO
typedef checksum_image_test     image_test_t;
typedef checksum_image_generate image_generate_t;
#else
typedef file_image_test         image_test_t;
typedef file_image_generate     image_generate_t;
#endif

#ifdef BOOST_GIL_GENERATE_REFERENCE_DATA
typedef image_generate_t        image_mgr_t;
#else
typedef image_test_t            image_mgr_t;
#endif


void test_image(const char* ref_checksum) {
    image_mgr_t mgr(ref_checksum);

    mgr.run();
    static_checks();
}

int main(int argc, char* argv[]) {

    const char* local_name = "gil_reference_checksums.txt";
    const char* name_from_status = "../libs/gil/test/gil_reference_checksums.txt";

    std::ifstream file_is_there(local_name);
    if (file_is_there) {
        test_image(local_name);
    } else {
        std::ifstream file_is_there(name_from_status);
        if (file_is_there)
            test_image(name_from_status);
        else {
            std::cerr << "Unable to open gil_reference_checksums.txt"<<std::endl;
            return 1;
        }
    }

    return 0;
}

