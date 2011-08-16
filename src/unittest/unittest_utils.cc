#include "unittest/unittest_utils.hpp"

#include <stdlib.h>

#include "arch/arch.hpp"

namespace unittest {

temp_file_t::temp_file_t(const char *tmpl) {
    size_t len = strlen(tmpl);
    filename = new char[len+1];
    memcpy(filename, tmpl, len+1);
    int fd = mkstemp(filename);
    guarantee_err(fd != -1, "Couldn't create a temporary file");
    close(fd);
}

temp_file_t::~temp_file_t() {
    unlink(filename);
    delete [] filename;
}

}  // namespace unittest


struct starter_t : public thread_message_t {
    thread_pool_t *tp;
    boost::function<void()> fun;
    starter_t(thread_pool_t *_tp, const boost::function<void()>& _fun) : tp(_tp), fun(_fun) { }
    void run() {
        fun();
        tp->shutdown();
    }
    void on_thread_switch() {
        coro_t::spawn_now(boost::bind(&starter_t::run, this));
    }
};

void run_in_thread_pool(const boost::function<void()>& fun) {
    thread_pool_t thread_pool(1);
    starter_t starter(&thread_pool, fun);
    thread_pool.run(&starter);
}
