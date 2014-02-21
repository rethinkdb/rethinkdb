#include "buffer_cache/types.hpp"

#include "debug.hpp"

void debug_print(printf_buffer_t *buf, block_magic_t magic) {
    buf->appendf("block_magic{");
    char *bytes = magic.bytes;
    debug_print_quoted_string(buf, reinterpret_cast<uint8_t *>(bytes), sizeof(magic.bytes));
    buf->appendf("}");
}
