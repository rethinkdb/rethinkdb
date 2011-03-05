#ifndef __IO_QUEUE_HPP__
#define __IO_QUEUE_HPP__

#include <libaio.h>

/*
* TODO!
*/
class io_queue_t {
public:
    virtual ~io_queue_t() { }

    virtual void push(iocb *request) = 0;
    virtual bool empty() = 0;
    virtual iocb *pull() = 0;
    virtual iocb *peek() = 0;
};

#endif /* __IO_QUEUE_HPP__ */
