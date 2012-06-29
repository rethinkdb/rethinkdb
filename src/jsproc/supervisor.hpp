#ifndef JSPROC_SUPERVISOR_HPP_
#define JSPROC_SUPERVISOR_HPP_

#include "utils.hpp"
#include <boost/scoped_ptr.hpp>

#include "containers/archive/fd_stream.hpp"

namespace jsproc {

// A handle to our supervisor process. Its public methods are threadsafe, but
// may involve switching threads temporarily.
class supervisor_t :
    private home_thread_mixin_t
{
  public:
    class info_t {
        friend class supervisor_t;
        int socket_fd;
    };

    // Must be called within a thread pool, exactly once.
    supervisor_t(const info_t &info);
    ~supervisor_t();

    // Called to create the supervisor process and make us its child. Must be
    // called *OUTSIDE* of a thread pool (ie. before run_in_thread_pool).
    //
    // On success, returns 0 and initializes `*info` in the child process, and
    // does not return in the parent (instead the parent runs the supervisor).
    //
    // On failure, returns -1 in the parent process (the child is dead or never
    // existed).
    static int spawn(info_t *info);

    // Spawns `job` off and initializes `stream` with a connection to the job.
    // Thread-safe, but connect() involves switching threads temporarily.
    template<typename instance_t>
    int spawn_job(const instance_t &job, boost::scoped_ptr<unix_socket_stream_t> &stream) {
        int res = connect(stream);
        if (!res) {
            res = send_job(stream.get(), job);
            // Close connected stream on error.
            if (res)
                stream.reset();
        }
        return res;
    }

  private:
    // Connects us to a worker. Private; used only by spawn_job.
    // Returns -1 on error.
    int connect(boost::scoped_ptr<unix_socket_stream_t> &stream);

    void install_on_thread(int i);
    void uninstall_on_thread(int i);

    // Called by spawn() in the supervisor process.
    static void exec_supervisor(pid_t child, int sockfd) THROWS_NOTHING;

  private:
    boost::scoped_ptr<unix_socket_stream_t> stream_;
};

} // namespace jsproc

#endif // JSPROC_SUPERVISOR_HPP_
