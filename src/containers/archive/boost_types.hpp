// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_BOOST_TYPES_HPP_
#define CONTAINERS_ARCHIVE_BOOST_TYPES_HPP_

#include "errors.hpp"
#include <boost/logic/tribool.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "containers/archive/archive.hpp"
#include "containers/archive/varint.hpp"

template <cluster_version_t W>
NORETURN void serialize(UNUSED write_message_t *wm,
                        UNUSED const boost::detail::variant::void_ &v) {
    unreachable("You cannot do serialize(write_message_t *, boost::detail::variant::void_ &).");
}

template <cluster_version_t W>
MUST_USE archive_result_t deserialize(UNUSED read_stream_t *s,
                                      UNUSED boost::detail::variant::void_ *v) {
    unreachable("You cannot do deserialize(read_stream_t *, boost::detail::variant::void_ *).");
}

template <cluster_version_t W, int N, class... Ts>
struct variant_serializer_t;

template <cluster_version_t W, int N, class... Ts>
struct variant_serializer_t<W, N, boost::detail::variant::void_, Ts...> : public variant_serializer_t<W, N> {

    explicit variant_serializer_t(write_message_t *wm)
        : variant_serializer_t<W, N>(wm) { }

    using variant_serializer_t<W, N>::operator();
};

template <cluster_version_t W, int N, class T, class... Ts>
struct variant_serializer_t<W, N, T, Ts...> : public variant_serializer_t<W, N + 1, Ts...> {

    explicit variant_serializer_t(write_message_t *wm)
        : variant_serializer_t<W, N + 1, Ts...>(wm) { }

    using variant_serializer_t<W, N + 1, Ts...>::operator();
    using variant_serializer_t<W, N + 1, Ts...>::wm_;

    typedef void result_type;

    void operator()(const T &x) {
        uint8_t n = N;
        serialize<W>(wm_, n);
        serialize<W>(wm_, x);
    }
};

template <cluster_version_t W, int N>
struct variant_serializer_t<W, N> {
    struct end_of_variant { };

    explicit variant_serializer_t(write_message_t *wm) : wm_(wm) { }

    void operator()(const end_of_variant&){
        unreachable("variant_serializer_t: no more types");
    }

    static const uint8_t size = N;
    write_message_t *wm_;

};

template <cluster_version_t W, BOOST_VARIANT_ENUM_PARAMS(class T)>
void serialize(write_message_t *wm, const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &x) {
    variant_serializer_t<W, 1, BOOST_VARIANT_ENUM_PARAMS(T)> visitor(wm);
    rassert(sizeof(visitor) == sizeof(write_message_t *));

    boost::apply_visitor(visitor, x);
}

template <cluster_version_t W, int N, class Variant, class... Ts>
struct variant_deserializer;

template <cluster_version_t W, int N, class Variant, class... Ts>
struct variant_deserializer<W, N, Variant, boost::detail::variant::void_, Ts...> {
    static MUST_USE archive_result_t deserialize_variant(int, read_stream_t *, Variant *) {
        return archive_result_t::RANGE_ERROR;
    }
};

template <cluster_version_t W, int N, class Variant, class T, class... Ts>
struct variant_deserializer<W, N, Variant, T, Ts...> {
    static MUST_USE archive_result_t deserialize_variant(int n, read_stream_t *s, Variant *x) {
        if (n == N) {
            *x = T();
            archive_result_t res = deserialize<W>(s, boost::get<T>(x));
            if (bad(res)) { return res; }

            return archive_result_t::SUCCESS;
        } else {
            return variant_deserializer<W, N + 1, Variant, Ts...>::deserialize_variant(n, s, x);
        }
    }
};

template <cluster_version_t W, int N, class Variant>
struct variant_deserializer<W, N, Variant> {
    static MUST_USE archive_result_t deserialize_variant(int, read_stream_t *, Variant *) {
        return archive_result_t::RANGE_ERROR;
    }
};

template <cluster_version_t W, BOOST_VARIANT_ENUM_PARAMS(class T)>
MUST_USE archive_result_t deserialize(read_stream_t *s, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> *x) {
    uint8_t n;
    archive_result_t res = deserialize<W>(s, &n);
    if (bad(res)) { return res; }

    return variant_deserializer<W, 1, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, BOOST_VARIANT_ENUM_PARAMS(T)>::deserialize_variant(n, s, x);
}


template <cluster_version_t W, class T>
void serialize(write_message_t *wm, const boost::optional<T> &x) {
    const T *ptr = x.get_ptr();
    bool exists = ptr;
    serialize<W>(wm, exists);
    if (exists) {
        serialize<W>(wm, *ptr);
    }
}

template <cluster_version_t W, class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, boost::optional<T> *x) {
    bool exists;

    archive_result_t res = deserialize<W>(s, &exists);
    if (bad(res)) { return res; }
    if (exists) {
        x->reset(T());
        res = deserialize<W>(s, x->get_ptr());
        return res;
    } else {
        x->reset();
        return archive_result_t::SUCCESS;
    }
}

template <cluster_version_t W>
void serialize(write_message_t *write_message, boost::tribool const &value) {
    int8_t mapping = -1;

    if (!value) {
        // `value` is false
        mapping = 0;
    } else if (value) {
        // `value` is true
        mapping = 1;
    } else {
        // `value` is indeterminate
        mapping = 2;
    }

    serialize<W>(write_message, mapping);
}

template <cluster_version_t W>
MUST_USE archive_result_t deserialize(read_stream_t *read_stream, boost::tribool *value) {
    int8_t mapping;

    archive_result_t result = deserialize<W>(read_stream, &mapping);
    if (bad(result)) {
        return result;
    }

    switch (mapping) {
        case 0:
            *value = false;
            break;
        case 1:
            *value = true;
            break;
        case 2:
            *value = boost::indeterminate;
            break;
        default:
            return archive_result_t::RANGE_ERROR;
    }

    return archive_result_t::SUCCESS;
}

#endif  // CONTAINERS_ARCHIVE_BOOST_TYPES_HPP_
