#ifndef BUFFER_CACHE_GLOBAL_PAGE_REPL_HPP_
#define BUFFER_CACHE_GLOBAL_PAGE_REPL_HPP_

#include "errors.hpp"

class global_page_repl_t {
public:
    global_page_repl_t();
    ~global_page_repl_t();

private:
    DISABLE_COPYING(global_page_repl_t);
};

#endif  // BUFFER_CACHE_GLOBAL_PAGE_REPL_HPP_
