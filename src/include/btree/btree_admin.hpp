
#ifndef __BTREE_ADMIN_HPP__
#define __BTREE_ADMIN_HPP__

#include <unistd.h>
#include <sys/types.h>
#include "arch/resource.hpp"
#include "config/args.hpp"
#include "utils.hpp"

template <class config_t>
class btree_admin {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    
public:
    static void create_db(resource_t fd) {
        int res = ftruncate(fd, IO_BUFFER_SIZE);
        check("Could not truncate db file", res != 0);

        char *buf = (char*)malloc_aligned(IO_BUFFER_SIZE, IO_BUFFER_SIZE);
        *((block_id_t*)buf) = serializer_t::null_block_id;

        res = write(fd, buf, IO_BUFFER_SIZE);
        check("Could not write root id", res != IO_BUFFER_SIZE);

        free(buf);
    }
};

#endif // __BTREE_ADMIN_HPP__

