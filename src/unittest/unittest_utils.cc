#include "unittest/unittest_utils.hpp"

#include <stdlib.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "do_on_thread.hpp"
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
    boost::function<void()> run;

    starter_t(thread_pool_t *_tp, const boost::function<void()>& _fun) : tp(_tp), run(boost::bind(&starter_t::run_wrapper, this, _fun)) { }
    void on_thread_switch() {
        spawn_on_thread(0, run);
    }
private:
    void run_wrapper(const boost::function<void()>& fun) {
        fun();
        tp->shutdown();
    }
};

void run_in_thread_pool(const boost::function<void()>& fun, int num_threads) {
    thread_pool_t thread_pool(num_threads, false);
    starter_t starter(&thread_pool, fun);
    thread_pool.run(&starter);
}
