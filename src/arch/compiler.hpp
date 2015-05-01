#ifndef ARCH_COMPILER_HPP_
#define ARCH_COMPILER_HPP_

#ifdef _MSC_VER
#define ATTR_ALIGNED(size) __declspec(align(size))
#else
#define ATTR_ALIGNED(size) __attribute__((aligned(size)))
#endif

#endif