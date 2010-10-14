
#ifndef __SERIALIZER_HPP__
#define __SERIALIZER_HPP__

#include "serializer/log/log_serializer.hpp"

#ifdef SEMANTIC_SERIALIZER_CHECK

#include "serializer/semantic_checking.hpp"
typedef semantic_checking_serializer_t<log_serializer_t> serializer_t;

#else

typedef log_serializer_t serializer_t;

#endif

#endif /* __SERIALIZER_HPP__ */
