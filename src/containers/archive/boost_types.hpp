#ifndef CONTAINERS_ARCHIVE_BOOST_TYPES_HPP_
#define CONTAINERS_ARCHIVE_BOOST_TYPES_HPP_

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/variant.hpp>

#include "containers/archive/archive.hpp"

inline
write_message_t &operator<<(UNUSED write_message_t &msg /* NOLINT */, UNUSED const boost::detail::variant::void_ &v) {
    unreachable("You cannot do operator<<(write_message_t &, boost::detail::variant::void_ &).");
}

inline
MUST_USE archive_result_t deserialize(UNUSED read_stream_t *s, UNUSED boost::detail::variant::void_ *v) {
    unreachable("You cannot do deserialize(read_stream_t *, boost::detail::variant::void_ *).");
}


#define ARCHIVE_VARIANT_SERIALIZE_VISITOR_METHOD(i)     \
    void operator()(const T##i &x /* NOLINT */) {       \
        uint8_t n = i;                                  \
        *(this->msg) << n;                              \
        *(this->msg) << x;                              \
    }

#define ARCHIVE_VARIANT_SERIALIZE_USING_DECL(i) \
    using v_##i##_t<ARCHIVE_TL##i>::operator()

namespace archive_nonsense {

template <class T20> struct v_20_t : public boost::static_visitor<void> {
    v_20_t() : msg(NULL) { }
    write_message_t *msg;

    ARCHIVE_VARIANT_SERIALIZE_VISITOR_METHOD(20);
private:
    DISABLE_COPYING(v_20_t);
};

#define ARCHIVE_CLASS_DECL(i, j)                                        \
    template <ARCHIVE_CL##i>                                            \
    struct v_##i##_t : public v_##j##_t<ARCHIVE_TL##j> {                \
        ARCHIVE_VARIANT_SERIALIZE_USING_DECL(j);                        \
        ARCHIVE_VARIANT_SERIALIZE_VISITOR_METHOD(i);                    \
    }


#define ARCHIVE_CL19 class T19, class T20
#define ARCHIVE_TL20 T20
ARCHIVE_CLASS_DECL(19, 20);
#define ARCHIVE_CL18 class T18, ARCHIVE_CL19
#define ARCHIVE_TL19 T19, ARCHIVE_TL20
ARCHIVE_CLASS_DECL(18, 19);
#define ARCHIVE_CL17 class T17, ARCHIVE_CL18
#define ARCHIVE_TL18 T18, ARCHIVE_TL19
ARCHIVE_CLASS_DECL(17, 18);
#define ARCHIVE_CL16 class T16, ARCHIVE_CL17
#define ARCHIVE_TL17 T17, ARCHIVE_TL18
ARCHIVE_CLASS_DECL(16, 17);
#define ARCHIVE_CL15 class T15, ARCHIVE_CL16
#define ARCHIVE_TL16 T16, ARCHIVE_TL17
ARCHIVE_CLASS_DECL(15, 16);
#define ARCHIVE_CL14 class T14, ARCHIVE_CL15
#define ARCHIVE_TL15 T15, ARCHIVE_TL16
ARCHIVE_CLASS_DECL(14, 15);
#define ARCHIVE_CL13 class T13, ARCHIVE_CL14
#define ARCHIVE_TL14 T14, ARCHIVE_TL15
ARCHIVE_CLASS_DECL(13, 14);
#define ARCHIVE_CL12 class T12, ARCHIVE_CL13
#define ARCHIVE_TL13 T13, ARCHIVE_TL14
ARCHIVE_CLASS_DECL(12, 13);
#define ARCHIVE_CL11 class T11, ARCHIVE_CL12
#define ARCHIVE_TL12 T12, ARCHIVE_TL13
ARCHIVE_CLASS_DECL(11, 12);
#define ARCHIVE_CL10 class T10, ARCHIVE_CL11
#define ARCHIVE_TL11 T11, ARCHIVE_TL12
ARCHIVE_CLASS_DECL(10, 11);
#define ARCHIVE_CL9 class T9, ARCHIVE_CL10
#define ARCHIVE_TL10 T10, ARCHIVE_TL11
ARCHIVE_CLASS_DECL(9, 10);
#define ARCHIVE_CL8 class T8, ARCHIVE_CL9
#define ARCHIVE_TL9 T9, ARCHIVE_TL10
ARCHIVE_CLASS_DECL(8, 9);
#define ARCHIVE_CL7 class T7, ARCHIVE_CL8
#define ARCHIVE_TL8 T8, ARCHIVE_TL9
ARCHIVE_CLASS_DECL(7, 8);
#define ARCHIVE_CL6 class T6, ARCHIVE_CL7
#define ARCHIVE_TL7 T7, ARCHIVE_TL8
ARCHIVE_CLASS_DECL(6, 7);
#define ARCHIVE_CL5 class T5, ARCHIVE_CL6
#define ARCHIVE_TL6 T6, ARCHIVE_TL7
ARCHIVE_CLASS_DECL(5, 6);
#define ARCHIVE_CL4 class T4, ARCHIVE_CL5
#define ARCHIVE_TL5 T5, ARCHIVE_TL6
ARCHIVE_CLASS_DECL(4, 5);
#define ARCHIVE_CL3 class T3, ARCHIVE_CL4
#define ARCHIVE_TL4 T4, ARCHIVE_TL5
ARCHIVE_CLASS_DECL(3, 4);
#define ARCHIVE_CL2 class T2, ARCHIVE_CL3
#define ARCHIVE_TL3 T3, ARCHIVE_TL4
ARCHIVE_CLASS_DECL(2, 3);
#define ARCHIVE_CL1 class T1, ARCHIVE_CL2
#define ARCHIVE_TL2 T2, ARCHIVE_TL3
ARCHIVE_CLASS_DECL(1, 2);

}  // namespace archive_nonsense

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
write_message_t &operator<<(write_message_t &msg, const boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> &x) {
    archive_nonsense::v_1_t<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> visitor;
    rassert(sizeof(visitor) == sizeof(write_message_t *));

    visitor.msg = &msg;

    boost::apply_visitor(visitor, x);

    return msg;
}

template <class T> struct archive_variant_deserialize_standin_t {
    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
    static MUST_USE archive_result_t do_the_deserialization(read_stream_t *s, boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *x) {
        T v;
        archive_result_t res = deserialize(s, &v);
        if (res) { return res; }
        *x = v;

        return ARCHIVE_SUCCESS;
    }
};

template <> struct archive_variant_deserialize_standin_t<boost::detail::variant::void_> {
    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
    static MUST_USE archive_result_t do_the_deserialization(UNUSED read_stream_t *s, UNUSED boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *x) {
        return ARCHIVE_RANGE_ERROR;
    }
};

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
MUST_USE archive_result_t deserialize(read_stream_t *s, boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *x) {
    uint8_t n;
    archive_result_t res = deserialize(s, &n);
    if (res) { return res; }
    if (!(1 <= n && n <= 20)) {
        return ARCHIVE_RANGE_ERROR;
    }

    switch (n) {
    case 1: { return archive_variant_deserialize_standin_t<T1>::do_the_deserialization(s, x); }
    case 2: { return archive_variant_deserialize_standin_t<T2>::do_the_deserialization(s, x); }
    case 3: { return archive_variant_deserialize_standin_t<T3>::do_the_deserialization(s, x); }
    case 4: { return archive_variant_deserialize_standin_t<T4>::do_the_deserialization(s, x); }
    case 5: { return archive_variant_deserialize_standin_t<T5>::do_the_deserialization(s, x); }
    case 6: { return archive_variant_deserialize_standin_t<T6>::do_the_deserialization(s, x); }
    case 7: { return archive_variant_deserialize_standin_t<T7>::do_the_deserialization(s, x); }
    case 8: { return archive_variant_deserialize_standin_t<T8>::do_the_deserialization(s, x); }
    case 9: { return archive_variant_deserialize_standin_t<T9>::do_the_deserialization(s, x); }
    case 10: { return archive_variant_deserialize_standin_t<T10>::do_the_deserialization(s, x); }
    case 11: { return archive_variant_deserialize_standin_t<T11>::do_the_deserialization(s, x); }
    case 12: { return archive_variant_deserialize_standin_t<T12>::do_the_deserialization(s, x); }
    case 13: { return archive_variant_deserialize_standin_t<T13>::do_the_deserialization(s, x); }
    case 14: { return archive_variant_deserialize_standin_t<T14>::do_the_deserialization(s, x); }
    case 15: { return archive_variant_deserialize_standin_t<T15>::do_the_deserialization(s, x); }
    case 16: { return archive_variant_deserialize_standin_t<T16>::do_the_deserialization(s, x); }
    case 17: { return archive_variant_deserialize_standin_t<T17>::do_the_deserialization(s, x); }
    case 18: { return archive_variant_deserialize_standin_t<T18>::do_the_deserialization(s, x); }
    case 19: { return archive_variant_deserialize_standin_t<T19>::do_the_deserialization(s, x); }
    case 20: { return archive_variant_deserialize_standin_t<T20>::do_the_deserialization(s, x); }

    default:
        unreachable("impossible to reach, we already returned -3");
    }

    unreachable("impossible to reach, we return from every case of the switch statement");
}


template <class T>
write_message_t &operator<<(write_message_t &msg, const boost::optional<T> &x) {
    const T *ptr = x.get_ptr();
    bool exists = ptr;
    msg << exists;
    if (exists) {
        msg << *ptr;
    }
    return msg;
}


template <class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, boost::optional<T> *x) {
    bool exists;
    //rassert(!x->get_ptr());

    archive_result_t res = deserialize(s, &exists);
    if (res) { return res; }
    if (exists) {
        x->reset(T());
        res = deserialize(s, x->get_ptr());
        return res;
    } else {
        x->reset();
        return ARCHIVE_SUCCESS;
    }
}


template <class K, class V>
write_message_t &operator<<(write_message_t &msg, const boost::ptr_map<K, V> &x) {
    msg << x.size();
    for (typename boost::ptr_map<K, V>::const_iterator it = x.begin(); it != x.end(); ++it) {
        msg << it->first;
        msg << *it->second;
    }
    return msg;
}

template <class K, class V>
MUST_USE archive_result_t deserialize(read_stream_t *s, boost::ptr_map<K, V> *x) {
    x->clear();

    typename boost::ptr_map<K, V>::size_type size;
    archive_result_t res = deserialize(s, &size);
    if (res != ARCHIVE_SUCCESS) {
        goto FAIL;
    }

    for (typename boost::ptr_map<K, V>::size_type i = 0; i < size; ++i) {
        K k;
        res = deserialize(s, &k);
        if (res != ARCHIVE_SUCCESS) {
            goto FAIL;
        }

        res = deserialize(s, &((*x)[k]));
        if (res != ARCHIVE_SUCCESS) {
            goto FAIL;
        }
    }

    return ARCHIVE_SUCCESS;

FAIL:
    x->clear();
    return res;
}

#endif  // CONTAINERS_ARCHIVE_BOOST_TYPES_HPP_
