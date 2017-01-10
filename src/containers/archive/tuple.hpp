#ifndef CONTAINERS_ARCHIVE_TUPLE_HPP_
#define CONTAINERS_ARCHIVE_TUPLE_HPP_

#include <tuple>

#include "containers/archive/archive.hpp"

// When there's library support, we can use the C++14 std::index_sequence.

template <class T>
struct emit_type {
    using type = T;
};

template <size_t... Is>
struct rindex_sequence { };

template <size_t N, size_t... Rest>
struct build_rindex_sequence
    : std::conditional<N == 0,
                       emit_type<rindex_sequence<Rest...>>,
                       build_rindex_sequence<N - 1, N - 1, Rest...>>::type { };

template <size_t N>
struct make_rindex_sequence : build_rindex_sequence<N>::type { };

template <cluster_version_t W, class tuple_type, size_t... Is>
size_t serialized_size_helper(const tuple_type &tup, rindex_sequence<Is...>) {
    (void)tup;
    size_t acc = 0;
    // We specify the array size, and add 1, to satisfy VS 2015
    // complaining about array of size zero.
    int ign[sizeof...(Is) + 1]
        = { ((acc += serialized_size<W>(std::get<Is>(tup))), 0)... };
    (void)ign;
    return acc;
}

template <cluster_version_t W, class... Args>
size_t serialized_size(const std::tuple<Args...> &tup) {
    return serialized_size_helper<W>(tup, make_rindex_sequence<sizeof...(Args)>());
}

template <cluster_version_t W, class tuple_type, size_t... Is>
void serialize_helper(write_message_t *wm, const tuple_type &tup, rindex_sequence<Is...>) {
    // Silence unused variable warning in gcc for tuple<>.
    (void)wm;
    (void)tup;
    // We specify the array size, and add 1, to satisfy VS 2015
    // complaining about array of size zero.
    int ign[sizeof...(Is) + 1]
        = { (serialize<W>(wm, std::get<Is>(tup)), 0)... };
    (void)ign;
}

template <cluster_version_t W, class... Args>
void serialize(write_message_t *wm, const std::tuple<Args...> &tup) {
    serialize_helper<W>(wm, tup, make_rindex_sequence<sizeof...(Args)>());
}

template <cluster_version_t W, class tuple_type, size_t... Is>
MUST_USE archive_result_t deserialize_helper(read_stream_t *s, tuple_type *tup, rindex_sequence<Is...>) {
    (void)s;
    (void)tup;
    archive_result_t res = archive_result_t::SUCCESS;
    int ign[sizeof...(Is) + 1]
        = { (res == archive_result_t::SUCCESS
             ? ((res = deserialize<W>(s, &std::get<Is>(*tup))), 0)
             : 0)... };
    (void)ign;
    return res;
}

template <cluster_version_t W, class... Args>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::tuple<Args...> *tup) {
    return deserialize_helper<W>(s, tup, make_rindex_sequence<sizeof...(Args)>());
}


#endif  // CONTAINERS_ARCHIVE_TUPLE_HPP_
