#include "containers/binary_blob.hpp"

#include "containers/printf_buffer.hpp"
#include "containers/archive/stl_types.hpp"

RDB_IMPL_ME_SERIALIZABLE_1(binary_blob_t, storage);


void debug_print(printf_buffer_t *buf, const binary_blob_t &blob) {
    const uint8_t *data = static_cast<const uint8_t *>(blob.data());
    const uint8_t *end = data + blob.size();

    buf->appendf("bb{");
    for (const uint8_t *p = data; p != end; ++p) {
        buf->appendf("%s%02X", (p == data ? "" : " "), *p);
    }
    buf->appendf("}");
}
