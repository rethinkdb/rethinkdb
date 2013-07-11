#include "buffer_cache/types.hpp"

void debugf_print(printf_buffer_t *buf, block_magic_t magic) {
    CT_ASSERT(sizeof(magic.bytes) == 4);
    buf->appendf("block_magic{%c,%c,%c,%c}",
                 magic.bytes[0], magic.bytes[1], magic.bytes[2], magic.bytes[3]);
}
