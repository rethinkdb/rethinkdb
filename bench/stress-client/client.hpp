#ifndef __STRESS_CLIENT_CLIENT_HPP__
#define __STRESS_CLIENT_CLIENT_HPP__

#include <pthread.h>
#include <stdint.h>
#include "op.hpp"
#include <queue>
#include <signal.h>

using namespace std;

/* Structure that represents a running client */
struct client_t {

    client_t(int _pipeline_limit = 0, int _ignore_protocol_errors = 0) :
        keep_running(false),
        total_freq(0),
        pipeline_limit(_pipeline_limit),
        ignore_protocol_errors(_ignore_protocol_errors),
        print_further_protocol_errors(true)
        { }

    void add_op(int freq, op_generator_t *op_gen) {
        ops.push_back(op_gen);
        freqs.push_back(freq);
        total_freq += freq;
    }

    void start() {

        assert(!keep_running);
        keep_running = true;

        int res = pthread_create(&thread, NULL, &client_t::run_client, (void*)this);
        if(res != 0) {
            fprintf(stderr, "Can't create thread\n");
            exit(-1);
        }
    }

    void stop() {
        assert(keep_running);

        spinlock.lock();
        keep_running = false;
        spinlock.unlock();

        int res = pthread_join(thread, NULL);
        if(res != 0) {
            fprintf(stderr, "Can't join on the thread\n");
            exit(-1);
        }
    }

    ~client_t() {
        assert(!keep_running);
    }

    // The ops that we are running against the database
    std::vector<op_generator_t *> ops;
    std::vector<int> freqs;   // One entry in freqs for each entry in ops
    int total_freq;

    int pipeline_limit;
    int ignore_protocol_errors;

private:
    // This spinlock protects keep_running from race conditions
    spinlock_t spinlock;
    pthread_t thread;

    // Main thread sets this to false in order to stop the client
    bool keep_running;

    bool print_further_protocol_errors;

    static void* run_client(void* data) {
        static_cast<client_t*>(data)->run();
    }

    void run() {

        spinlock.lock();
        std::queue<op_t *> outstanding_ops;
        while(keep_running) {
            spinlock.unlock();

            /* Select which operation to perform */
            int op_counter = xrandom(0, total_freq - 1);
            op_t *op_to_do = NULL;
            for (int i = 0; i < ops.size(); i++) {
                if (op_counter < freqs[i]) {
                    op_to_do = ops[i]->generate();
                    break;
                } else {
                    op_counter -= freqs[i];
                }
            }

            if (!op_to_do) {
                fprintf(stderr, "Something went horribly wrong with the random op choice.\n");
                exit(-1);
            }

            try {
                op_to_do->start();
                outstanding_ops.push(op_to_do);

                while (outstanding_ops.size() > pipeline_limit) {
                    outstanding_ops.front()->end();
                    outstanding_ops.pop();
                }

            } catch (protocol_error_t &e) {
                if (!ignore_protocol_errors) {
                    fprintf(stderr, "Protocol error: %s\n", e.c_str());
                    throw e;
                } else {
                    if (print_further_protocol_errors) {
                        fprintf(stderr, "Protocol error: %s\n", e.c_str());
                        fprintf(stderr, "Ignored. Further errors on this connection will not be printed.\n");
                        print_further_protocol_errors = false;
                    }
                }
            }

            spinlock.lock();
        }
        spinlock.unlock();
    }
};

#endif  // __STRESS_CLIENT_CLIENT_HPP__

