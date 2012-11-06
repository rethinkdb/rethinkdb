#include "arch/types.hpp"

file_account_t::file_account_t(file_t *par, int pri, int outstanding_requests_limit) :
    parent(par),
    account(parent->create_account(pri, outstanding_requests_limit)) { }

file_account_t::~file_account_t() {
    parent->destroy_account(account);
}

