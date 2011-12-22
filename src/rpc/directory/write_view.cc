#include "rpc/directory/write_view.hpp"

directory_write_service_t::our_value_lock_acq_t::our_value_lock_acq_t(directory_write_service_t *dir) THROWS_NOTHING {
    dir->assert_thread();
    acq.reset(dir->get_our_value_lock());
}

void directory_write_service_t::our_value_lock_acq_t::assert_is_holding(directory_write_service_t *dir) THROWS_NOTHING {
    dir->assert_thread();
    acq.assert_is_holding(dir->get_our_value_lock());
}
