#ifndef CONCURRENCY_CACHE_LINE_PADDED_HPP_
#define CONCURRENCY_CACHE_LINE_PADDED_HPP_

#define CACHE_LINE_SIZE                           64

/* Pad a value to the size of one or multiple cache lines to avoid false sharing.
 */
#define COMPUTE_PADDING_SIZE(value, alignment) \
    (alignment) - ((((value) + (alignment) - 1) % (alignment)) + 1)

template <class value_t>
struct cache_line_padded_t {
    cache_line_padded_t() { }
    template <class... Args>
    explicit cache_line_padded_t(Args &&... args) : value(std::forward<Args>(args)...) { }
    value_t value;
    char padding[COMPUTE_PADDING_SIZE(sizeof(value_t), CACHE_LINE_SIZE)];
};
#undef COMPUTE_PADDING_SIZE


#endif  // CONCURRENCY_CACHE_LINE_PADDED_HPP_
