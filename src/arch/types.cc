#include "arch/types.hpp"

#include "utils.hpp"

address_in_use_exc_t::address_in_use_exc_t(const char* hostname, int port) throw () {
    if (port == 0) {
        info = strprintf("Could not establish sockets on all selected local addresses using the same port");
    } else {
        info = strprintf("The address at %s:%d is reserved or already in use", hostname, port);
    }
}

file_account_t::file_account_t(file_t *par, int pri, int outstanding_requests_limit) :
    parent(par),
    account(parent->create_account(pri, outstanding_requests_limit)) { }

file_account_t::~file_account_t() {
    parent->destroy_account(account);
}

void linux_iocallback_t::on_io_failure(int errsv, int64_t offset, int64_t count) {
    crash("I/O operation failed.  (%s) (offset = %" PRIi64 ", count = %" PRIi64 ")",
          errno_string(errsv).c_str(), offset, count);
}
