#ifndef __SERIALIZER_CONFIG_HPP__
#define __SERIALIZER_CONFIG_HPP__

#include "serializer/log/log_serializer.hpp"

#ifdef SEMANTIC_SERIALIZER_CHECK

#include "serializer/semantic_checking.hpp"
typedef semantic_checking_serializer_t<log_serializer_t> standard_serializer_t;

#else

typedef log_serializer_t standard_serializer_t;

#endif

#endif /* __SERIALIZER_CONFIG_HPP__ */
