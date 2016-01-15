#ifndef SRC_ARCH_WINDOWS_STUB_SYS_UIO_H_
#define SRC_ARCH_WINDOWS_STUB_SYS_UIO_H_

// A windows stub for <uio.h>

#include "windows.hpp"

struct iovec {
	void  *iov_base;
	size_t iov_len;
};

#endif  // SRC_ARCH_WINDOWS_STUB_SYS_UIO_H_
