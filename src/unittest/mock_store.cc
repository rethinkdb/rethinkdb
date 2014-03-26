#include "unittest/mock_store.hpp"

namespace unittest {

void mock_store_t::new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out) {
    assert_thread();
    fifo_enforcer_read_token_t token = token_source_.enter_read();
    token_out->create(&token_sink_, token);
}

void mock_store_t::new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out) THROWS_NOTHING {
    assert_thread();
    fifo_enforcer_write_token_t token = token_source_.enter_write();
    token_out->create(&token_sink_, token);
}





}  // namespace unittest
