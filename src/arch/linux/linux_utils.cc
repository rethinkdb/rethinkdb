
#include <sys/syscall.h>
#include <unistd.h>
#include "arch/linux/linux_utils.hpp"

int _gettid() {
    return syscall(SYS_gettid);
}
