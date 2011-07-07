#ifndef _UTILS_ALIGN_HPP_
#define _UTILS_ALIGN_HPP_

template<typename T>
static inline T align_downto(T n, T align) { return n - (n % align); }

template<typename T>
static inline T align_upto(T n, T align) { return n % align ? align_downto(n, align) + align : n; }

#endif
