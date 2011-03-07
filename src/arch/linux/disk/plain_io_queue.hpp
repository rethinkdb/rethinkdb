#ifndef __PLAIN_IO_QUEUE_HPP__
#define	__PLAIN_IO_QUEUE_HPP__

#include <list>
#include <libaio.h>
#include "arch/linux/disk/io_queue.hpp"


/*
* For testing purposes...
*/
class plain_io_queue_t : public io_queue_t {
public:    
    virtual ~plain_io_queue_t() {
        rassert(empty());
    }

    virtual void push(iocb *request) {
        queue.push_back(request);
    }
    virtual bool empty() {
        return queue.empty();
    }
    virtual iocb *pull() {
        rassert(!empty());
        iocb *request = queue.front();
        queue.erase(queue.begin());
        return request;
    }
    virtual iocb *peek() {
        rassert(!empty());
        return queue.front();
    }

private:
    std::list<iocb *> queue;
};

#endif	/* __PLAIN_IO_QUEUE_HPP__ */

