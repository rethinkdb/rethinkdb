
#include <sys/syscall.h>
#include <unistd.h>
#include "linux_utils.hpp"

int _gettid() {
    return syscall(SYS_gettid);
}
