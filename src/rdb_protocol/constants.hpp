// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_CONSTANTS_HPP_
#define RDB_PROTOCOL_CONSTANTS_HPP_

namespace ql {

#ifndef NDEBUG
const size_t batch_size = KILOBYTE;
#else
const size_t batch_size = MEGABYTE / 4;
#endif // NDEBUG

}

#endif  // RDB_PROTOCOL_CONSTANTS_HPP_
