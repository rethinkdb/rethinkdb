#include "jsproc/example.hpp"

#include <cstring>              // strerror
#include <sys/types.h>
#include <sys/socket.h>

#include <v8.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/starter.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/logger.hpp"
#include "containers/archive/socket_stream.hpp"
#include "jsproc/job.hpp"
#include "jsproc/pool.hpp"
#include "jsproc/spawner.hpp"
#include "rpc/serialize_macros.hpp"

class js_eval_job_t :
    public jsproc::auto_job_t<js_eval_job_t>
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
    virtual void run_job(control_t *control) {
        control->log("Running js_eval_job_t");

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
        if (send_write_message(control, &msg)) {
            control->log("js_eval_job_t: Could not send back evaluated JS.");
        }
    }

  private:
    std::string js_src;

  public:
    RDB_MAKE_ME_SERIALIZABLE_1(js_src);
};

void run_rethinkdb_js(const jsproc::spawner_t::info_t &info, bool *result) {
    struct killer_message_t : linux_thread_message_t {
        virtual void on_thread_switch() {
            // NB. This is a terrible way to handle shut-down, and is only for
            // example purposes (in particular, a clean shutdown would require
            // killing worker processes by destructing pool_group).
            fprintf(stderr, "SIGINT received, exiting...\n");
            abort();
        }
    } msg;
    linux_thread_pool_t::thread_pool->set_interrupt_message(&msg);

    fprintf(stderr, "ENGINE PROC: %d\n", getpid());

    jsproc::pool_group_t pool_group(info, jsproc::pool_group_t::DEFAULTS);
    jsproc::pool_t *pool = pool_group.get();

    local_issue_tracker_t tracker;
    log_writer_t writer("example-log", &tracker);

    // Shell loop.
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
        jsproc::job_handle_t handle;
        if(-1 == pool->spawn_job(&job, &handle)) {
            printf("!! could not spawn job\n");
            break;
        }

        // Read back the result.
        printf("waiting for result...\n");
        js_eval_job_t::result_t result;
        archive_result_t ares = deserialize(&handle, &result);
        if (ARCHIVE_SUCCESS != ares) {
            printf("!! could not deserialize result: %s\n",
                   ares == ARCHIVE_SOCK_ERROR ? "socket error" :
                   ares == ARCHIVE_SOCK_EOF ? "EOF" :
                   ares == ARCHIVE_RANGE_ERROR ? "range error" :
                   "unknown error");
            break;
        }

        // Print the result.
        printf(result.success ? "success!\nresult: %s\n" : "failure! :(\nreason: %s\n",
               result.message.c_str());
    }

    *result = -1;
    printf("run_rethinkdb_js exiting\n");
}

int main_rethinkdb_js(int argc, char *argv[]) {
    (void) argc; (void) argv;

    jsproc::spawner_t::info_t spawner_info;
    guarantee(-1 != jsproc::spawner_t::create(&spawner_info));

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_js, spawner_info, &result), 1);
    printf("main_rethinkdb_js exiting\n");
    return result ? 0 : 1;
}
