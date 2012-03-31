#ifndef CONTAINERS_ARCHIVE_BOOST_TYPES_HPP_
#define CONTAINERS_ARCHIVE_BOOST_TYPES_HPP_

#include "errors.hpp"
#include <boost/variant.hpp>

#include "containers/archive/primitives.hpp"

write_message_t &operator<<(UNUSED write_message_t &msg, UNUSED const boost::detail::variant::void_ &v) {
    unreachable("You cannot do operator<<(write_message_t &msg, const boost::detail::variant::void_ &v).");
}


#define ARCHIVE_VARIANT_VISITOR_METHOD(i)       \
    void operator()(const T##i &x) {            \
        uint8_t n = i;                          \
        *(this->msg) << n;                      \
        *(this->msg) << x;                      \
    }

#define ARCHIVE_USING_DECL(i)                   \
    using v_##i##_t<ARCHIVE_TL##i>::operator()

namespace archive_nonsense {

template <class T20> struct v_20_t : public boost::static_visitor<void> {
    v_20_t() : msg(NULL) { }
    write_message_t *msg;

    ARCHIVE_VARIANT_VISITOR_METHOD(20);
private:
    DISABLE_COPYING(v_20_t);
};


#define ARCHIVE_CL19 class T19, class T20
#define ARCHIVE_TL20 T20
template <ARCHIVE_CL19> struct v_19_t : public v_20_t<T20> { ARCHIVE_USING_DECL(20); ARCHIVE_VARIANT_VISITOR_METHOD(19); };
#define ARCHIVE_CL18 class T18, ARCHIVE_CL19
#define ARCHIVE_TL19 T19, ARCHIVE_TL20
template <ARCHIVE_CL18> struct v_18_t : public v_19_t<ARCHIVE_TL19> { ARCHIVE_USING_DECL(19); ARCHIVE_VARIANT_VISITOR_METHOD(18); };
#define ARCHIVE_CL17 class T17, ARCHIVE_CL18
#define ARCHIVE_TL18 T18, ARCHIVE_TL19
template <ARCHIVE_CL17> struct v_17_t : public v_18_t<ARCHIVE_TL18> { ARCHIVE_USING_DECL(18); ARCHIVE_VARIANT_VISITOR_METHOD(17); };
#define ARCHIVE_CL16 class T16, ARCHIVE_CL17
#define ARCHIVE_TL17 T17, ARCHIVE_TL18
template <ARCHIVE_CL16> struct v_16_t : public v_17_t<ARCHIVE_TL17> { ARCHIVE_USING_DECL(17); ARCHIVE_VARIANT_VISITOR_METHOD(16); };
#define ARCHIVE_CL15 class T15, ARCHIVE_CL16
#define ARCHIVE_TL16 T16, ARCHIVE_TL17
template <ARCHIVE_CL15> struct v_15_t : public v_16_t<ARCHIVE_TL16> { ARCHIVE_USING_DECL(16); ARCHIVE_VARIANT_VISITOR_METHOD(15); };
#define ARCHIVE_CL14 class T14, ARCHIVE_CL15
#define ARCHIVE_TL15 T15, ARCHIVE_TL16
template <ARCHIVE_CL14> struct v_14_t : public v_15_t<ARCHIVE_TL15> { ARCHIVE_USING_DECL(15); ARCHIVE_VARIANT_VISITOR_METHOD(14); };
#define ARCHIVE_CL13 class T13, ARCHIVE_CL14
#define ARCHIVE_TL14 T14, ARCHIVE_TL15
template <ARCHIVE_CL13> struct v_13_t : public v_14_t<ARCHIVE_TL14> { ARCHIVE_USING_DECL(14); ARCHIVE_VARIANT_VISITOR_METHOD(13); };
#define ARCHIVE_CL12 class T12, ARCHIVE_CL13
#define ARCHIVE_TL13 T13, ARCHIVE_TL14
template <ARCHIVE_CL12> struct v_12_t : public v_13_t<ARCHIVE_TL13> { ARCHIVE_USING_DECL(13); ARCHIVE_VARIANT_VISITOR_METHOD(12); };
#define ARCHIVE_CL11 class T11, ARCHIVE_CL12
#define ARCHIVE_TL12 T12, ARCHIVE_TL13
template <ARCHIVE_CL11> struct v_11_t : public v_12_t<ARCHIVE_TL12> { ARCHIVE_USING_DECL(12); ARCHIVE_VARIANT_VISITOR_METHOD(11); };
#define ARCHIVE_CL10 class T10, ARCHIVE_CL11
#define ARCHIVE_TL11 T11, ARCHIVE_TL12
template <ARCHIVE_CL10> struct v_10_t : public v_11_t<ARCHIVE_TL11> { ARCHIVE_USING_DECL(11); ARCHIVE_VARIANT_VISITOR_METHOD(10); };
#define ARCHIVE_CL9 class T9, ARCHIVE_CL10
#define ARCHIVE_TL10 T10, ARCHIVE_TL11
template <ARCHIVE_CL9> struct v_9_t : public v_10_t<ARCHIVE_TL10> { ARCHIVE_USING_DECL(10); ARCHIVE_VARIANT_VISITOR_METHOD(9); };
#define ARCHIVE_CL8 class T8, ARCHIVE_CL9
#define ARCHIVE_TL9 T9, ARCHIVE_TL10
template <ARCHIVE_CL8> struct v_8_t : public v_9_t<ARCHIVE_TL9> { ARCHIVE_USING_DECL(9); ARCHIVE_VARIANT_VISITOR_METHOD(8); };
#define ARCHIVE_CL7 class T7, ARCHIVE_CL8
#define ARCHIVE_TL8 T8, ARCHIVE_TL9
template <ARCHIVE_CL7> struct v_7_t : public v_8_t<ARCHIVE_TL8> { ARCHIVE_USING_DECL(8); ARCHIVE_VARIANT_VISITOR_METHOD(7); };
#define ARCHIVE_CL6 class T6, ARCHIVE_CL7
#define ARCHIVE_TL7 T7, ARCHIVE_TL8
template <ARCHIVE_CL6> struct v_6_t : public v_7_t<ARCHIVE_TL7> { ARCHIVE_USING_DECL(7); ARCHIVE_VARIANT_VISITOR_METHOD(6); };
#define ARCHIVE_CL5 class T5, ARCHIVE_CL6
#define ARCHIVE_TL6 T6, ARCHIVE_TL7
template <ARCHIVE_CL5> struct v_5_t : public v_6_t<ARCHIVE_TL6> { ARCHIVE_USING_DECL(6); ARCHIVE_VARIANT_VISITOR_METHOD(5); };
#define ARCHIVE_CL4 class T4, ARCHIVE_CL5
#define ARCHIVE_TL5 T5, ARCHIVE_TL6
template <ARCHIVE_CL4> struct v_4_t : public v_5_t<ARCHIVE_TL5> { ARCHIVE_USING_DECL(5); ARCHIVE_VARIANT_VISITOR_METHOD(4); };
#define ARCHIVE_CL3 class T3, ARCHIVE_CL4
#define ARCHIVE_TL4 T4, ARCHIVE_TL5
template <ARCHIVE_CL3> struct v_3_t : public v_4_t<ARCHIVE_TL4> { ARCHIVE_USING_DECL(4); ARCHIVE_VARIANT_VISITOR_METHOD(3); };
#define ARCHIVE_CL2 class T2, ARCHIVE_CL3
#define ARCHIVE_TL3 T3, ARCHIVE_TL4
template <ARCHIVE_CL2> struct v_2_t : public v_3_t<ARCHIVE_TL3> { ARCHIVE_USING_DECL(3); ARCHIVE_VARIANT_VISITOR_METHOD(2); };
#define ARCHIVE_CL1 class T1, ARCHIVE_CL2
#define ARCHIVE_TL2 T2, ARCHIVE_TL3
template <ARCHIVE_CL1> struct v_1_t : public v_2_t<ARCHIVE_TL2> { ARCHIVE_USING_DECL(2); ARCHIVE_VARIANT_VISITOR_METHOD(1); };

}  // namespace archive_nonsense

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
write_message_t &operator<<(write_message_t &msg, const boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> &x) {
    archive_nonsense::v_1_t<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> visitor;

    visitor.msg = &msg;

    boost::apply_visitor(visitor, x);

    return msg;
}




#endif  // CONTAINERS_ARCHIVE_BOOST_TYPES_HPP_
