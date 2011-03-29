
#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <pthread.h>
#include <stdint.h>
#include "op.hpp"

using namespace std;

/* Structure that represents a running client */
struct client_t {

    client_t() : keep_running(false), total_freq(0) { }

    void add_op(int freq, op_t *op) {
        ops.push_back(op);
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
    std::vector<op_t *> ops;
    std::vector<int> freqs;   // One entry in freqs for each entry in ops
    int total_freq;

private:
    // This spinlock protects keep_running from race conditions
    spinlock_t spinlock;
    pthread_t thread;

    // Main thread sets this to false in order to stop the client
    bool keep_running;

    static void* run_client(void* data) {
        static_cast<client_t*>(data)->run();
    }

    void run() {

        spinlock.lock();
        while(keep_running) {
            spinlock.unlock();

            /* Select which operation to perform */
            int op_counter = xrandom(0, total_freq - 1);
            op_t *op_to_do = NULL;
            for (int i = 0; i < ops.size(); i++) {
                if (op_counter < freqs[i]) {
                    op_to_do = ops[i];
                    break;
                } else {
                    op_counter -= freqs[i];
                }
            }
            if (!op_to_do) {
                fprintf(stderr, "Something went horribly wrong with the random op choice.\n");
                exit(-1);
            }

            op_to_do->run();

            spinlock.lock();
        }
        spinlock.unlock();
    }
};

#endif // __CLIENT_HPP__

