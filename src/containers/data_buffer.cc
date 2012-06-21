#include "containers/data_buffer.hpp"
#include "utils.hpp"


void debug_print(append_only_printf_buffer_t *buf, const intrusive_ptr_t<data_buffer_t>& ptr) {
    if (!ptr.has()) {
        buf->appendf("databuf_ptr{null}");
    } else {
        buf->appendf("databuf_ptr{");
        const char *bytes = ptr->buf();
        debug_print_quoted_string(buf, reinterpret_cast<const uint8_t *>(bytes), ptr->size());
        buf->appendf("}");
    }
}
