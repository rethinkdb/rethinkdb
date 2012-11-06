#include "arch/types.hpp"

linux_file_account_t::linux_file_account_t(file_t *par, int pri, int outstanding_requests_limit) :
    parent(par),
    account(parent->create_account(pri, outstanding_requests_limit)) { }

linux_file_account_t::~linux_file_account_t() {
    parent->destroy_account(account);
}

