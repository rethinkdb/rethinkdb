#ifndef CONTAINERS_ARCHIVE_PRIMITIVES_HPP_
#define CONTAINERS_ARCHIVE_PRIMITIVES_HPP_

#include "containers/archive.hpp"

#define ARCHIVE_PRIM_WRITE(typ)                                         \
    write_message_t &operator<<(write_message_t &msg, typ x) {          \
        union {                                                         \
            typ v;                                                      \
            char buf[sizeof(typ)];                                      \
        } u;                                                            \
        u.v = x;                                                        \
        msg.append(u.buf, sizeof(typ));                                 \
        return msg;                                                     \
    }

ARCHIVE_PRIM_WRITE(int8_t);
ARCHIVE_PRIM_WRITE(uint8_t);
ARCHIVE_PRIM_WRITE(int16_t);
ARCHIVE_PRIM_WRITE(uint16_t);
ARCHIVE_PRIM_WRITE(int32_t);
ARCHIVE_PRIM_WRITE(uint32_t);
ARCHIVE_PRIM_WRITE(int64_t);
ARCHIVE_PRIM_WRITE(uint64_t);







#endif  // CONTAINERS_ARCHIVE_PRIMITIVES_HPP_
