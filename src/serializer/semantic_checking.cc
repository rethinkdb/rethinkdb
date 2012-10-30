// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifdef SEMANTIC_SERIALIZER_CHECK

// Instantiate the semantic checking serializer.

#include "serializer/log/log_serializer.hpp"
#include "serializer/semantic_checking.tcc"

template class semantic_checking_serializer_t<log_serializer_t>;

#endif
