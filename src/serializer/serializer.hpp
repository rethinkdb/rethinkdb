
#ifndef __SERIALIZER_HPP__
#define __SERIALIZER_HPP__

#ifndef NDEBUG

#include "serializer/semantic_checking.hpp"
#include "serializer/log/log_serializer.hpp"
typedef semantic_checking_serializer_t<log_serializer_t> serializer_t;

#else

#include "serializer/log/log_serializer.hpp"
typedef log_serializer_t serializer_t;

#endif

#endif /* __SERIALIZER_HPP__ */