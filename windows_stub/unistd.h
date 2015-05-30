#pragma once

// RSI
#include "windows.hpp"
#include <io.h>
#include <sys/types.h>
#include <stdio.h>
#include <BaseTsd.h>

typedef int gid_t;
typedef int uid_t;

inline ssize_t pread(long fd, void* buf, size_t count, off_t offset) {
	int res = _lseek(fd, offset, SEEK_SET);
	if (res < 0) return res;
	return _read(fd, buf, count);
}

inline ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
	int res = _lseek(fd, offset, SEEK_SET);
	if (res < 0) return res;
	return _write(fd, buf, count);
}

#define F_OK 00

#define close _close