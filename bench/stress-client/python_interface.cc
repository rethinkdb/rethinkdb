#include "python_interface.h"
#include "client.hpp"

void *client_create(const char *server_str, int id, int num_clients, const char **opts) {

    load_t *load = new load_t();
    while (*opts) {
        const char *key = *(opts++);
        const char *value = *(opts++);
        if (strcmp(key, "op_ratios") == 0) {
            load->op_ratios.parse(value);
        } else if (strcmp(key, "keys") == 0) {
            load->keys.parse(value);
        } else if (strcmp(key, "values") == 0) {
            load->values.parse(value);
        } else if (strcmp(key, "batch_factor") == 0) {
            load->batch_factor.parse(value);
        } else if (strcmp(key, "distr") == 0) {
            if (strcmp(value, "uniform") == 0) {
                load->distr = rnd_uniform_t;
            } else if (strcmp(value, "normal") == 0) {
                load->distr = rnd_normal_t;
            } else {
                fprintf(stderr, "Unrecognized distribution '%s'\n", value);
                exit(-1);
            }
        } else if (strcmp(key, "mu") == 0) {
            load->mu = atoi(value);
        } else {
            fprintf(stderr, "Unrecognized configuration key '%s'\n", key);
            exit(-1);
        }
    }

    server_t server;
    server.parse(server_str);
    protocol_t *proto = server.connect(load);

    client_t *client = new client_t(load, proto, NULL, id, num_clients);

    return reinterpret_cast<void*>(client);
}

void client_start(void *client) {
    reinterpret_cast<client_t *>(client)->start();
}

void client_poll(void *client,
    int *queries_out, int *inserts_minus_deletes_out,
    float *worst_latency_out,
    int *n_latencies_inout, float *latencies_out) {

    query_stats_t stats;
    reinterpret_cast<client_t *>(client)->poll(&stats);

    if (queries_out) *queries_out = stats.queries;
    if (inserts_minus_deletes_out) *inserts_minus_deletes_out = stats.inserts_minus_deletes;
    if (worst_latency_out) *worst_latency_out = ticks_to_secs(stats.worst_latency);

    if (n_latencies_inout) {
        int n_latencies = std::min((int)stats.latency_samples.size(), *n_latencies_inout);
        *n_latencies_inout = n_latencies;
        for (int i = 0; i < n_latencies; i++) {
            latencies_out[i] = ticks_to_secs(stats.latency_samples.samples[i]);
        }
    }
}

void client_stop(void *client) {
    reinterpret_cast<client_t *>(client)->stop();
}

void client_destroy(void *vclient) {
    client_t *client = reinterpret_cast<client_t *>(vclient);
    delete client->proto;
    delete client->load;
    delete client;
}
