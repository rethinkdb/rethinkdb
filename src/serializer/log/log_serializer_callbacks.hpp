
#ifndef __LOG_SERIALIZER_CALLBACKS_HPP__
#define __LOG_SERIALIZER_CALLBACKS_HPP__

struct _write_txn_callback_t {
    virtual void on_serializer_write_txn() = 0;
};
struct _write_block_callback_t {
    virtual void on_serializer_write_block() = 0;
};

#endif
