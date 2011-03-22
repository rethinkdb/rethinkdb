
#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <pthread.h>
#include "protocol.hpp"
#include "sqlite_protocol.hpp"
#include <stdint.h>
#include "assert.h"

#define update_salt 6830

using namespace std;

struct query_stats_t {

    int queries;
    ticks_t worst_latency;
    reservoir_sample_t<ticks_t> latency_samples;

    query_stats_t() : queries(0), worst_latency(0) { }

    void push(ticks_t latency, bool enable_latency_samples, int batch_count) {
        queries += batch_count;
        worst_latency = std::max(worst_latency, latency);
        if (enable_latency_samples) {
            for (int i = 0; i < batch_count; i++) latency_samples.push(latency);
        }
    }

    void aggregate(const query_stats_t &other) {
        queries += other.queries;
        worst_latency = std::max(worst_latency, other.worst_latency);
        latency_samples += other.latency_samples;
    }
};

/* There is a 1:1:1 correspondence between client_t's, threads, and key-ranges. There has
to be a 1:1 correspondence between threads and key ranges to avoid race conditions, and a
client_t is the object that wraps a thread/keyrange.

There is a 1:many correspondence between client_t's and op_t's. An op_t represents a type
of operation that can be run against a client.

For example, suppose that we wanted to make three clients perform reads and writes against
a server, with a 5:2 read:write ratio. We could write this code:

    protocol_t *connections[3];
    client_t *clients[3];
    read_op_t *read_ops[3];
    insert_op_t *write_ops[3];

    for (int i = 0; i < 3; i++) {
        connections[i] = server.connect();
        clients[i] = new client_t(i, 3, ...);
        read_ops[i] = new read_op_t(clients[i], 5, connections[i], ...);
        insert_ops[i] = new insert_op_t(clients[i], 2, connections[i], ...);
        clients[i]->start();
    }

Note that we must create a separate set of op_ts for each client. Also, we must create a
separate protocol_t for every client. If we later want to see how many reads each of our
three clients has done, we could do this:

    for (int i = 0; i < 3; i++) {
        clients[i]->spinlock.lock();   // This is only necessary if the client is running
        printf("Client %d has done %d reads.\n", i, read_ops[i]->stats.queries);
        clients[i]->spinlock.unlock();
    }

*/

struct op_t;

/* Structure that represents a running client */
struct client_t {

    client_t(int id, int num_clients, distr_t keys, rnd_distr_t distr, int mu, sqlite_protocol_t *sp) :
        id(id), num_clients(num_clients), min_seed(0), max_seed(0), keys(keys), sqlite(sp)
    {
        keep_running = false;
        total_freq = 0;
        _rnd = xrandom_create(distr, mu);

        id_salt = id;
        id_salt += id_salt << 32;

        max_value_size = 0;
    }

    void start() {

        value_buffer.first = new char[max_value_size];
        memset(value_buffer.first, 'A', max_value_size);   // Is this necessary?

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

        delete[] value_buffer.first;
    }

    ~client_t() {
        assert(!keep_running);
    }

    int id, num_clients;
    int min_seed, max_seed;

    distr_t keys;
    rnd_gen_t _rnd;

    sqlite_protocol_t *sqlite;

    // This spinlock governs all the communications variables between the client and main threads.
    spinlock_t spinlock;

    uint64_t id_salt;

    int max_value_size;
    payload_t value_buffer;   // For the convenience of op_t's

    // The ops that we are running against the database
    std::vector<op_t *> ops;
    int total_freq;

    void gen_key(payload_t *pl, int keyn) {
        keys.toss(pl, keyn ^ id_salt, id, num_clients - 1);
    }

private:
    pthread_t thread;

    // Main thread sets this to false in order to stop the client
    bool keep_running;

    static void* run_client(void* data) {
        static_cast<client_t*>(data)->run();
    }

    void run();
};

struct op_t {

    op_t(client_t *c, int freq, int max_value_size) : client(c), freq(freq)
    {
        c->ops.push_back(this);
        c->total_freq += freq;
        c->max_value_size = std::max(c->max_value_size, max_value_size);
    }

    client_t *client;

    /* The probability that this op will be run is its frequency divided by the sum
    of all the frequencies of all the ops on this client. */
    int freq;

    void push_stats(float latency, int count) {
        client->spinlock.lock();
        stats.push(latency, enable_latency_samples, count);
        client->spinlock.unlock();
    }

    query_stats_t stats;
    bool enable_latency_samples;

    virtual void run() = 0;
};

// We declare this function down here to break the circular dependency between
// client_t and op_t.
inline void client_t::run() {

    spinlock.lock();
    while(keep_running) {
        spinlock.unlock();

        /* Select which operation to perform */
        int op_counter = xrandom(0, total_freq - 1);
        op_t *op_to_do = NULL;
        for (int i = 0; i < ops.size(); i++) {
            if (op_counter < ops[i]->freq) {
                op_to_do = ops[i];
                break;
            } else {
                op_counter -= ops[i]->freq;
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

#endif // __CLIENT_HPP__

