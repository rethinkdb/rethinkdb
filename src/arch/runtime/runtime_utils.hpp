#ifndef __ARCH_RUNTIME_RUNTIME_UTILS_HPP__
#define __ARCH_RUNTIME_RUNTIME_UTILS_HPP__

#include "errors.hpp"
#include "containers/intrusive_list.hpp"

typedef int fd_t;
#define INVALID_FD fd_t(-1)

class linux_thread_message_t :
    public intrusive_list_node_t<linux_thread_message_t>
{
public:
    virtual void on_thread_switch() = 0;
protected:
    virtual ~linux_thread_message_t() {}
};

int get_cpu_count();

#endif // __ARCH_RUNTIME_RUNTIME_UTILS_HPP__
