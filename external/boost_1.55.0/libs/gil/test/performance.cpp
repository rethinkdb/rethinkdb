/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).

    See http://opensource.adobe.com/gil for most recent version including documentation.
*/

/*************************************************************************************************/

/// \file
/// \brief GIL performance test suite
/// \date 2007 \n Last updated on February 12, 2007
///
/// Available tests:
///    fill_pixels() on rgb8_image_t with rgb8_pixel_t
///    fill_pixels() on rgb8_planar_image_t with rgb8_pixel_t
///    fill_pixels() on rgb8_image_t with bgr8_pixel_t
///    fill_pixels() on rgb8_planar_image_t with bgr8_pixel_t
///    for_each_pixel() on rgb8_image_t
///    for_each_pixel() on rgb8_planar_t
///    copy_pixels() between rgb8_image_t and rgb8_image_t
///    copy_pixels() between rgb8_image_t and bgr8_image_t
///    copy_pixels() between rgb8_planar_image_t and rgb8_planar_image_t
///    copy_pixels() between rgb8_image_t and rgb8_planar_image_t
///    copy_pixels() between rgb8_planar_image_t and rgb8_image_t
///    transform_pixels() between rgb8_image_t and rgb8_image_t
///    transform_pixels() between rgb8_planar_image_t and rgb8_planar_image_t
///    transform_pixels() between rgb8_planar_image_t and rgb8_image_t
///    transform_pixels() between rgb8_image_t and rgb8_planar_image_t

#include <cstddef>
#include <ctime>
#include <iostream>
#include <boost/gil/pixel.hpp>
#include <boost/gil/planar_pixel_iterator.hpp>
#include <boost/gil/planar_pixel_reference.hpp>
#include <boost/gil/iterator_from_2d.hpp>
#include <boost/gil/step_iterator.hpp>
#include <boost/gil/rgb.hpp>
#include <boost/gil/image_view.hpp>
#include <boost/gil/image.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/algorithm.hpp>

using namespace boost::gil;

// returns time in milliseconds per call
template <typename Op>
double measure_time(Op op, std::size_t num_loops) {
    clock_t begin=clock();
    for (std::size_t ii=0; ii<num_loops; ++ii) op();
    return double(clock()-begin)/double(num_loops);
}

// image dimension
std::size_t width=1000, height=400;

// macros for standard GIL views
#define RGB_VIEW(T) image_view<memory_based_2d_locator<memory_based_step_iterator<pixel<T,rgb_layout_t>*> > >
#define BGR_VIEW(T) image_view<memory_based_2d_locator<memory_based_step_iterator<pixel<T,bgr_layout_t>*> > >
#define RGB_PLANAR_VIEW(T) image_view<memory_based_2d_locator<memory_based_step_iterator<planar_pixel_iterator<T*,rgb_t> > > >

template <typename View, typename P>
struct fill_gil_t {
    View _v;
    P _p;
    fill_gil_t(const View& v_in,const P& p_in) : _v(v_in), _p(p_in) {}
    void operator()() const {fill_pixels(_v,_p);}
};
template <typename View, typename P> struct fill_nongil_t;
template <typename T, typename P>
struct fill_nongil_t<RGB_VIEW(T), P> {
    typedef RGB_VIEW(T) View;
    View _v;
    P _p;
    fill_nongil_t(const View& v_in,const P& p_in) : _v(v_in), _p(p_in) {}
    void operator()() const {
        T* first=(T*)_v.row_begin(0);
        T* last=first+_v.size()*3;
        while(first!=last) {
            first[0]=boost::gil::at_c<0>(_p);
            first[1]=boost::gil::at_c<1>(_p);
            first[2]=boost::gil::at_c<2>(_p);
            first+=3;
        }
    }
};
template <typename T1, typename T2>
struct fill_nongil_t<RGB_VIEW(T1), pixel<T2,bgr_layout_t> > {
    typedef RGB_VIEW(T1) View;
    typedef pixel<T2,bgr_layout_t> P;
    View _v;
    P _p;
    fill_nongil_t(const View& v_in,const P& p_in) : _v(v_in), _p(p_in) {}
    void operator()() const {
        T1* first=(T1*)_v.row_begin(0);
        T1* last=first+_v.size()*3;
        while(first!=last) {
            first[0]=boost::gil::at_c<2>(_p);
            first[1]=boost::gil::at_c<1>(_p);
            first[2]=boost::gil::at_c<0>(_p);
            first+=3;
        }
    }
};
template <typename T1, typename T2>
struct fill_nongil_t<RGB_PLANAR_VIEW(T1), pixel<T2,rgb_layout_t> > {
    typedef RGB_PLANAR_VIEW(T1) View;
    typedef pixel<T2,rgb_layout_t> P;
    View _v;
    P _p;
    fill_nongil_t(const View& v_in,const P& p_in) : _v(v_in), _p(p_in) {}
    void operator()() const {
        std::size_t size=_v.size();
        T1* first;
        first=(T1*)boost::gil::at_c<0>(_v.row_begin(0));
        std::fill(first,first+size,boost::gil::at_c<0>(_p));
        first=(T1*)boost::gil::at_c<1>(_v.row_begin(0));
        std::fill(first,first+size,boost::gil::at_c<1>(_p));
        first=(T1*)boost::gil::at_c<2>(_v.row_begin(0));
        std::fill(first,first+size,boost::gil::at_c<2>(_p));
    }
};

template <typename T1, typename T2>
struct fill_nongil_t<RGB_PLANAR_VIEW(T1), pixel<T2,bgr_layout_t> > {
    typedef RGB_PLANAR_VIEW(T1) View;
    typedef pixel<T2,bgr_layout_t> P;
    View _v;
    P _p;
    fill_nongil_t(const View& v_in,const P& p_in) : _v(v_in), _p(p_in) {}
    void operator()() const {
        std::size_t size=_v.size();
        T1* first;
        first=(T1*)boost::gil::at_c<0>(_v.row_begin(0));
        std::fill(first,first+size,boost::gil::at_c<2>(_p));
        first=(T1*)boost::gil::at_c<1>(_v.row_begin(0));
        std::fill(first,first+size,boost::gil::at_c<1>(_p));
        first=(T1*)boost::gil::at_c<2>(_v.row_begin(0));
        std::fill(first,first+size,boost::gil::at_c<1>(_p));
    }
};

template <typename View, typename P>
void test_fill(std::size_t trials) {
    image<typename View::value_type, is_planar<View>::value> im(width,height);
    std::cout << "GIL: "<< measure_time(fill_gil_t<View,P>(view(im),P()),trials) << std::endl;
    std::cout << "Non-GIL: "<< measure_time(fill_nongil_t<View,P>(view(im),P()),trials) << std::endl;
};

template <typename T>
struct rgb_fr_t {
    void operator()(pixel<T,rgb_layout_t>& p) const {p[0]=0;p[1]=1;p[2]=2;}
    void operator()(const planar_pixel_reference<T&,rgb_t>& p) const {p[0]=0;p[1]=1;p[2]=2;}
};
template <typename View, typename F>
struct for_each_gil_t {
    View _v;
    F _f;
    for_each_gil_t(const View& v_in,const F& f_in) : _v(v_in), _f(f_in) {}
    void operator()() const {for_each_pixel(_v,_f);}
};
template <typename View, typename F> struct for_each_nongil_t;
template <typename T, typename T2>
struct for_each_nongil_t<RGB_VIEW(T), rgb_fr_t<T2> > {
    typedef RGB_VIEW(T) View;
    typedef rgb_fr_t<T2> F;
    View _v;
    F _f;
    for_each_nongil_t(const View& v_in,const F& f_in) : _v(v_in), _f(f_in) {}
    void operator()() const {
        T* first=(T*)_v.row_begin(0);
        T* last=first+_v.size()*3;
        while(first!=last) {
            first[0]=0;
            first[1]=1;
            first[2]=2;
            first+=3;
        }
    }
};
template <typename T1, typename T2>
struct for_each_nongil_t<RGB_PLANAR_VIEW(T1), rgb_fr_t<T2> > {
    typedef RGB_PLANAR_VIEW(T1) View;
    typedef rgb_fr_t<T2> F;
    View _v;
    F _f;
    for_each_nongil_t(const View& v_in,const F& f_in) : _v(v_in), _f(f_in) {}
    void operator()() const {
        T1 *first0, *first1, *first2, *last0;
        first0=(T1*)boost::gil::at_c<0>(_v.row_begin(0));
        first1=(T1*)boost::gil::at_c<1>(_v.row_begin(0));
        first2=(T1*)boost::gil::at_c<2>(_v.row_begin(0));
        last0=first0+_v.size();
        while(first0!=last0) {
            *first0++=0;
            *first1++=1;
            *first2++=2;
        }
    }
};

template <typename View, typename F>
void test_for_each(std::size_t trials) {
    image<typename View::value_type, is_planar<View>::value> im(width,height);
    std::cout << "GIL: "<<measure_time(for_each_gil_t<View,F>(view(im),F()),trials) << std::endl;
    std::cout << "Non-GIL: "<<measure_time(for_each_nongil_t<View,F>(view(im),F()),trials) << std::endl;
}

// copy
template <typename View1, typename View2>
struct copy_gil_t {
    View1 _v1;
    View2 _v2;
    copy_gil_t(const View1& v1_in,const View2& v2_in) : _v1(v1_in), _v2(v2_in) {}
    void operator()() const {copy_pixels(_v1,_v2);}
};
template <typename View1, typename View2> struct copy_nongil_t;
template <typename T1, typename T2>
struct copy_nongil_t<RGB_VIEW(T1),RGB_VIEW(T2)> {
    typedef RGB_VIEW(T1) View1;
    typedef RGB_VIEW(T2) View2;
    View1 _v1;
    View2 _v2;
    copy_nongil_t(const View1& v1_in,const View2& v2_in) : _v1(v1_in), _v2(v2_in) {}
    void operator()() const {
        T1* first1=(T1*)_v1.row_begin(0);
        T1* last1=first1+_v1.size()*3;
        T2* first2=(T2*)_v2.row_begin(0);
        std::copy(first1,last1,first2);
    }
};
template <typename T1, typename T2>
struct copy_nongil_t<RGB_VIEW(T1),BGR_VIEW(T2)> {
    typedef RGB_VIEW(T1) View1;
    typedef BGR_VIEW(T2) View2;
    View1 _v1;
    View2 _v2;
    copy_nongil_t(const View1& v1_in,const View2& v2_in) : _v1(v1_in), _v2(v2_in) {}
    void operator()() const {
        T1* first1=(T1*)_v1.row_begin(0);
        T1* last1=first1+_v1.size()*3;
        T2* first2=(T2*)_v2.row_begin(0);
        while(first1!=last1) {
            first2[2]=first1[0];
            first2[1]=first1[1];
            first2[0]=first1[2];
            first1+=3; first2+=3;
        }
    }
};
template <typename T1, typename T2>
struct copy_nongil_t<RGB_PLANAR_VIEW(T1),RGB_PLANAR_VIEW(T2)> {
    typedef RGB_PLANAR_VIEW(T1) View1;
    typedef RGB_PLANAR_VIEW(T2) View2;
    View1 _v1;
    View2 _v2;
    copy_nongil_t(const View1& v1_in,const View2& v2_in) : _v1(v1_in), _v2(v2_in) {}
    void operator()() const {
        std::size_t size=_v1.size();
        T1* first10=(T1*)boost::gil::at_c<0>(_v1.row_begin(0));
        T1* first11=(T1*)boost::gil::at_c<1>(_v1.row_begin(0));
        T1* first12=(T1*)boost::gil::at_c<2>(_v1.row_begin(0));
        T2* first20=(T2*)boost::gil::at_c<0>(_v2.row_begin(0));
        T2* first21=(T2*)boost::gil::at_c<1>(_v2.row_begin(0));
        T2* first22=(T2*)boost::gil::at_c<2>(_v2.row_begin(0));
        std::copy(first10,first10+size,first20);
        std::copy(first11,first11+size,first21);
        std::copy(first12,first12+size,first22);
    }
};
template <typename T1, typename T2>
struct copy_nongil_t<RGB_VIEW(T1),RGB_PLANAR_VIEW(T2)> {
    typedef RGB_VIEW(T1) View1;
    typedef RGB_PLANAR_VIEW(T2) View2;
    View1 _v1;
    View2 _v2;
    copy_nongil_t(const View1& v1_in,const View2& v2_in) : _v1(v1_in), _v2(v2_in) {}
    void operator()() const {
        T1* first=(T1*)_v1.row_begin(0);
        T1* last=first+_v1.size()*3;
        T2* first0=(T2*)boost::gil::at_c<0>(_v2.row_begin(0));
        T2* first1=(T2*)boost::gil::at_c<1>(_v2.row_begin(0));
        T2* first2=(T2*)boost::gil::at_c<2>(_v2.row_begin(0));
        while(first!=last) {
            *first0++=first[0];
            *first1++=first[1];
            *first2++=first[2];
            first+=3;
        }
    }
};
template <typename T1, typename T2>
struct copy_nongil_t<RGB_PLANAR_VIEW(T1),RGB_VIEW(T2)> {
    typedef RGB_PLANAR_VIEW(T1) View1;
    typedef RGB_VIEW(T2) View2;
    View1 _v1;
    View2 _v2;
    copy_nongil_t(const View1& v1_in,const View2& v2_in) : _v1(v1_in), _v2(v2_in) {}
    void operator()() const {
        T1* first=(T1*)_v2.row_begin(0);
        T1* last=first+_v2.size()*3;
        T2* first0=(T2*)boost::gil::at_c<0>(_v1.row_begin(0));
        T2* first1=(T2*)boost::gil::at_c<1>(_v1.row_begin(0));
        T2* first2=(T2*)boost::gil::at_c<2>(_v1.row_begin(0));
        while(first!=last) {
            first[0]=*first0++;
            first[1]=*first1++;
            first[2]=*first2++;
            first+=3;
        }
    }
};
template <typename View1, typename View2>
void test_copy(std::size_t trials) {
    image<typename View1::value_type, is_planar<View1>::value> im1(width,height);
    image<typename View2::value_type, is_planar<View2>::value> im2(width,height);
    std::cout << "GIL: "    <<measure_time(copy_gil_t<View1,View2>(view(im1),view(im2)),trials) << std::endl;
    std::cout << "Non-GIL: "<<measure_time(copy_nongil_t<View1,View2>(view(im1),view(im2)),trials) << std::endl;
}

// transform()
template <typename T,typename Pixel>
struct bgr_to_rgb_t {
    pixel<T,rgb_layout_t> operator()(const Pixel& p) const {
        return pixel<T,rgb_layout_t>(T(get_color(p,blue_t())*0.1f),
                                     T(get_color(p,green_t())*0.2f),
                                     T(get_color(p,red_t())*0.3f));
    }
};
template <typename View1, typename View2, typename F>
struct transform_gil_t {
    View1 _v1;
    View2 _v2;
    F _f;
    transform_gil_t(const View1& v1_in,const View2& v2_in,const F& f_in) : _v1(v1_in),_v2(v2_in),_f(f_in) {}
    void operator()() const {transform_pixels(_v1,_v2,_f);}
};
template <typename View1, typename View2, typename F> struct transform_nongil_t;
template <typename T1, typename T2, typename F>
struct transform_nongil_t<RGB_VIEW(T1),RGB_VIEW(T2),F> {
    typedef RGB_VIEW(T1) View1;
    typedef RGB_VIEW(T2) View2;
    View1 _v1;
    View2 _v2;
    F _f;
    transform_nongil_t(const View1& v1_in,const View2& v2_in,const F& f_in) : _v1(v1_in),_v2(v2_in),_f(f_in) {}
    void operator()() const {
        T1* first1=(T1*)_v1.row_begin(0);
        T2* first2=(T1*)_v2.row_begin(0);
        T1* last1=first1+_v1.size()*3;
        while(first1!=last1) {
            first2[0]=T2(first1[2]*0.1f);
            first2[1]=T2(first1[1]*0.2f);
            first2[2]=T2(first1[0]*0.3f);
            first1+=3; first2+=3;
        }
    }
};
template <typename T1, typename T2, typename F>
struct transform_nongil_t<RGB_PLANAR_VIEW(T1),RGB_PLANAR_VIEW(T2),F> {
    typedef RGB_PLANAR_VIEW(T1) View1;
    typedef RGB_PLANAR_VIEW(T2) View2;
    View1 _v1;
    View2 _v2;
    F _f;
    transform_nongil_t(const View1& v1_in,const View2& v2_in,const F& f_in) : _v1(v1_in),_v2(v2_in),_f(f_in) {}
    void operator()() const {
        T1* first10=(T1*)boost::gil::at_c<0>(_v1.row_begin(0));
        T1* first11=(T1*)boost::gil::at_c<1>(_v1.row_begin(0));
        T1* first12=(T1*)boost::gil::at_c<2>(_v1.row_begin(0));
        T1* first20=(T2*)boost::gil::at_c<0>(_v2.row_begin(0));
        T1* first21=(T2*)boost::gil::at_c<1>(_v2.row_begin(0));
        T1* first22=(T2*)boost::gil::at_c<2>(_v2.row_begin(0));
        T1* last10=first10+_v1.size();
        while(first10!=last10) {
            *first20++=T2(*first12++*0.1f);
            *first21++=T2(*first11++*0.2f);
            *first22++=T2(*first10++*0.3f);
        }
    }
};
template <typename T1, typename T2, typename F>
struct transform_nongil_t<RGB_VIEW(T1),RGB_PLANAR_VIEW(T2),F> {
    typedef RGB_VIEW(T1) View1;
    typedef RGB_PLANAR_VIEW(T2) View2;
    View1 _v1;
    View2 _v2;
    F _f;
    transform_nongil_t(const View1& v1_in,const View2& v2_in,const F& f_in) : _v1(v1_in),_v2(v2_in),_f(f_in) {}
    void operator()() const {
        T1* first1=(T1*)_v1.row_begin(0);
        T1* last1=first1+_v1.size()*3;
        T1* first20=(T2*)boost::gil::at_c<0>(_v2.row_begin(0));
        T1* first21=(T2*)boost::gil::at_c<1>(_v2.row_begin(0));
        T1* first22=(T2*)boost::gil::at_c<2>(_v2.row_begin(0));
        while(first1!=last1) {
            *first20++=T2(first1[2]*0.1f);
            *first21++=T2(first1[1]*0.2f);
            *first22++=T2(first1[0]*0.3f);
            first1+=3;
        }
    }
};
template <typename T1, typename T2, typename F>
struct transform_nongil_t<RGB_PLANAR_VIEW(T1),RGB_VIEW(T2),F> {
    typedef RGB_PLANAR_VIEW(T1) View1;
    typedef RGB_VIEW(T2) View2;
    View1 _v1;
    View2 _v2;
    F _f;
    transform_nongil_t(const View1& v1_in,const View2& v2_in,const F& f_in) : _v1(v1_in),_v2(v2_in),_f(f_in) {}
    void operator()() const {
        T1* first10=(T1*)boost::gil::at_c<0>(_v1.row_begin(0));
        T1* first11=(T1*)boost::gil::at_c<1>(_v1.row_begin(0));
        T1* first12=(T1*)boost::gil::at_c<2>(_v1.row_begin(0));
        T2* first2=(T1*)_v2.row_begin(0);
        T1* last2=first2+_v1.size()*3;
        while(first2!=last2) {
            first2[0]=T2(*first12++*0.1f);
            first2[1]=T2(*first11++*0.2f);
            first2[2]=T2(*first10++*0.3f);
            first2+=3;
        }
    }
};

template <typename View1, typename View2, typename F>
void test_transform(std::size_t trials) {
    image<typename View1::value_type, is_planar<View1>::value> im1(width,height);
    image<typename View2::value_type, is_planar<View2>::value> im2(width,height);
    std::cout << "GIL: "    <<measure_time(transform_gil_t<View1,View2,F>(view(im1),view(im2),F()),trials) << std::endl;
    std::cout << "Non-GIL: "<<measure_time(transform_nongil_t<View1,View2,F>(view(im1),view(im2),F()),trials) << std::endl;
}

int main() {
#ifdef NDEBUG
    std::size_t num_trials=1000;
#else
    std::size_t num_trials=1;
#endif

    // fill()
    std::cout<<"test fill_pixels() on rgb8_image_t with rgb8_pixel_t"<<std::endl;
    test_fill<rgb8_view_t,rgb8_pixel_t>(num_trials);
    std::cout<<std::endl;

    std::cout<<"test fill_pixels() on rgb8_planar_image_t with rgb8_pixel_t"<<std::endl;
    test_fill<rgb8_planar_view_t,rgb8_pixel_t>(num_trials);
    std::cout<<std::endl;

    std::cout<<"test fill_pixels() on rgb8_image_t with bgr8_pixel_t"<<std::endl;
    test_fill<rgb8_view_t,bgr8_pixel_t>(num_trials);
    std::cout<<std::endl;

    std::cout<<"test fill_pixels() on rgb8_planar_image_t with bgr8_pixel_t"<<std::endl;
    test_fill<rgb8_planar_view_t,bgr8_pixel_t>(num_trials);
    std::cout<<std::endl;

    // for_each()
    std::cout<<"test for_each_pixel() on rgb8_image_t"<<std::endl;
    test_for_each<rgb8_view_t,rgb_fr_t<bits8> >(num_trials);
    std::cout<<std::endl;

    std::cout<<"test for_each_pixel() on rgb8_planar_image_t"<<std::endl;
    test_for_each<rgb8_planar_view_t,rgb_fr_t<bits8> >(num_trials);
    std::cout<<std::endl;

    // copy()
    std::cout<<"test copy_pixels() between rgb8_image_t and rgb8_image_t"<<std::endl;
    test_copy<rgb8_view_t,rgb8_view_t>(num_trials);
    std::cout<<std::endl;

    std::cout<<"test copy_pixels() between rgb8_image_t and bgr8_image_t"<<std::endl;
    test_copy<rgb8_view_t,bgr8_view_t>(num_trials);
    std::cout<<std::endl;

    std::cout<<"test copy_pixels() between rgb8_planar_image_t and rgb8_planar_image_t"<<std::endl;
    test_copy<rgb8_planar_view_t,rgb8_planar_view_t>(num_trials);
    std::cout<<std::endl;

    std::cout<<"test copy_pixels() between rgb8_image_t and rgb8_planar_image_t"<<std::endl;
    test_copy<rgb8_view_t,rgb8_planar_view_t>(num_trials);
    std::cout<<std::endl;

    std::cout<<"test copy_pixels() between rgb8_planar_image_t and rgb8_image_t"<<std::endl;
    test_copy<rgb8_planar_view_t,rgb8_view_t>(num_trials);
    std::cout<<std::endl;

    // transform()
    std::cout<<"test transform_pixels() between rgb8_image_t and rgb8_image_t"<<std::endl;
    test_transform<rgb8_view_t,rgb8_view_t,bgr_to_rgb_t<bits8,pixel<bits8,rgb_layout_t> > >(num_trials);
    std::cout<<std::endl;

    std::cout<<"test transform_pixels() between rgb8_planar_image_t and rgb8_planar_image_t"<<std::endl;
    test_transform<rgb8_planar_view_t,rgb8_planar_view_t,bgr_to_rgb_t<bits8,planar_pixel_reference<bits8,rgb_t> > >(num_trials);
    std::cout<<std::endl;

    std::cout<<"test transform_pixels() between rgb8_image_t and rgb8_planar_image_t"<<std::endl;
    test_transform<rgb8_view_t,rgb8_planar_view_t,bgr_to_rgb_t<bits8,pixel<bits8,rgb_layout_t> > >(num_trials);
    std::cout<<std::endl;

    std::cout<<"test transform_pixels() between rgb8_planar_image_t and rgb8_image_t"<<std::endl;
    test_transform<rgb8_planar_view_t,rgb8_view_t,bgr_to_rgb_t<bits8,planar_pixel_reference<bits8,rgb_t> > >(num_trials);
    std::cout<<std::endl;

    return 0;
}
