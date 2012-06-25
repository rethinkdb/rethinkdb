#include "jsproc/example.hpp"

#include <cstring>              // strerror
#include <sys/types.h>
#include <sys/socket.h>

#include <v8.h>

#include "containers/archive/fd_stream.hpp"
#include "jsproc/job.hpp"
#include "jsproc/worker.hpp"
#include "rpc/serialize_macros.hpp"

class js_eval_job_t : public jsproc::job_t
{
  public:
    js_eval_job_t() {}
    js_eval_job_t(std::string src) : js_src(src) {}

    struct result_t {
        bool success;
        std::string message;
        RDB_MAKE_ME_SERIALIZABLE_2(success, message);
    };

  private:
    void eval(v8::Handle<v8::Context> cx, result_t *result) {
        v8::Context::Scope cx_scope(cx);
        v8::HandleScope handle_scope;
        v8::Handle<v8::String> src = v8::String::New(js_src.c_str());

        v8::TryCatch try_catch;
        v8::Handle<v8::Script> script = v8::Script::Compile(src);
        if (script.IsEmpty()) {
            result->message = "script compilation failed";
            return;
        }

        v8::Handle<v8::Value> value = script->Run();
        if (value.IsEmpty()) {
            result->message = "script execution failed";
            return;
        }

        v8::String::Utf8Value str(value);
        if (!*str) {
            result->message = "string conversion of result failed";
            return;
        }

        result->success = true;
        result->message = std::string(*str);
    }

  public:
    virtual void run_job(jsproc::job_result_t *result, UNUSED read_stream_t *input, write_stream_t *output) {
        printf("running job\n");

        result_t res;
        res.success = false;
        res.message = "";

        // Evaluate the script
        v8::Persistent<v8::Context> cx = v8::Context::New();
        eval(cx, &res);
        cx.Dispose();

        // Send back the result.
        write_message_t msg;
        msg << res;
        if (!send_write_message(output, &msg))
            result->type = jsproc::JOB_SUCCESS;
        else
            result->type = jsproc::JOB_WRITE_FAILURE;
    }

  private:
    std::string js_src;

  public:
    RDB_MAKE_ME_SERIALIZABLE_1(js_src);
};

int fork_worker(pid_t *pid, int *fd) {
    int fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (res) return -1;

    *fd = fds[0];
    *pid = fork();

    if (*pid == -1) {
        guarantee_err(0 == close(fds[0]), "could not close fd");
        guarantee_err(0 == close(fds[1]), "could not close fd");
        return -1;
    }

    if (!*pid) {
        // We're the child.
        guarantee_err(0 == close(fds[0]), "could not close fd");
        jsproc::exec_worker(fds[1]);
        unreachable();
    }

    // We're the parent
    guarantee(pid);
    guarantee_err(0 == close(fds[1]), "could not close fd");
    return 0;
}

int main_rethinkdb_js(int argc, char *argv[]) {
    (void) argc; (void) argv;

    pid_t pid;
    int fd;
    guarantee_err (0 == fork_worker(&pid, &fd), "could not spawn worker");
    unix_socket_stream_t stream(fd, new blocking_fd_watcher_t());

    char *line = NULL;
    size_t line_sz = 0;

    for (;;) {
        // Prompt for some javascript to evaluate.
        printf("> ");
        ssize_t res = getline(&line, &line_sz, stdin);
        if (res == -1) {
            printf("\n!! could not get line\n");
            break;
        }

        // Send a job that evaluates the javascript.
        js_eval_job_t job(line);
        if (-1 == jsproc::send_job(&stream, job)) {
            printf("!! could not send job\n");
            break;
        }

        // Read back the result.
        js_eval_job_t::result_t result;
        if (ARCHIVE_SUCCESS != deserialize(&stream, &result)) {
            printf("!! could not deserialize result\n");
            break;
        }

        // Print the result.
        printf(result.success ? "success!\nresult: %s\n" : "failure! :(\nreason: %s\n",
               result.message.c_str());
    }

    // debugf("main exiting\n");
    return -1;
}
