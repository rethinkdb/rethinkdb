#include "python_interface.h"
#include "client.hpp"
#include "read_ops.hpp"
#include "write_ops.hpp"
#include "mysql_protocol.hpp"

void *protocol_create(const char *server_str) {
    server_t server;
    server.parse(server_str);
    return static_cast<void*>(server.connect());
}

void protocol_destroy(void *vprotocol) {
    delete static_cast<protocol_t*>(vprotocol);
}

void *client_create(int id, int num_clients, int keysize_min, int keysize_max) {
    return static_cast<void*>(new client_t(
        id, num_clients,
        distr_t(keysize_min, keysize_max),
        rnd_uniform_t, 1, NULL));
}

void client_start(void *vclient) {
    static_cast<client_t*>(vclient)->start();
}

void client_lock(void *vclient) {
    static_cast<client_t*>(vclient)->spinlock.lock();
}

void client_unlock(void *vclient) {
    static_cast<client_t*>(vclient)->spinlock.unlock();
}

void client_stop(void *vclient) {
    static_cast<client_t*>(vclient)->stop();
}

void client_destroy(void *vclient) {
    delete static_cast<client_t*>(vclient);
}

void *op_create_read(void *vclient, void *vprotocol, int freq, int batchfactor_min, int batchfactor_max) {
    return static_cast<void*>(new read_op_t(
        static_cast<client_t*>(vclient),
        freq,
        static_cast<protocol_t*>(vprotocol),
        distr_t(batchfactor_min, batchfactor_max)
        ));
}

void *op_create_insert(void *vclient, void *vprotocol, int freq, int size_min, int size_max) {
    return static_cast<void*>(new insert_op_t(
        static_cast<client_t*>(vclient),
        freq,
        static_cast<protocol_t*>(vprotocol),
        distr_t(size_min, size_max)
        ));
}

void *op_create_update(void *vclient, void *vprotocol, int freq, int size_min, int size_max) {
    return static_cast<void*>(new update_op_t(
        static_cast<client_t*>(vclient),
        freq,
        static_cast<protocol_t*>(vprotocol),
        distr_t(size_min, size_max)
        ));
}

void *op_create_delete(void *vclient, void *vprotocol, int freq) {
    return static_cast<void*>(new delete_op_t(
        static_cast<client_t*>(vclient),
        freq,
        static_cast<protocol_t*>(vprotocol)
        ));
}

void *op_create_appendprepend(void *vclient, void *vprotocol, int freq, int is_append, int size_min, int size_max) {
    return static_cast<void*>(new append_prepend_op_t(
        static_cast<client_t*>(vclient),
        freq,
        static_cast<protocol_t*>(vprotocol),
        is_append != 0,
        distr_t(size_min, size_max)
        ));
}

void op_poll(void *vop, int *queries_out, float *worstlatency_out, int *samples_count_inout, float *samples_out) {
    // Assume that the Python script already locked the spinlock
    query_stats_t *stats = &static_cast<op_t*>(vop)->stats;

    if (queries_out) {
        *queries_out = stats->queries;
    }

    if (worstlatency_out) {
        *worstlatency_out = stats->worst_latency;
    }

    if (samples_count_inout && samples_out) {
        int samples_we_have = stats->latency_samples.size();
        int samples_we_need = *samples_count_inout = std::min(samples_we_have, *samples_count_inout);
        /* We have to be smart about delivering the samples so that if they request less
        samples than we have, we deliver a random representative sample. */
        while (samples_we_need > 0) {
            bool take_this_sample;
            if (samples_we_have == samples_we_need) {
                /* We are taking every sample */
                take_this_sample = true;
            } else if (samples_we_have > samples_we_need) {
                /* Roll a die to determine whether or not to take this sample */
                take_this_sample = xrandom(0, samples_we_have-1) < samples_we_need;
            }
            if (take_this_sample) {
                samples_out[--samples_we_need] = stats->latency_samples.samples[--samples_we_have];
            } else {
                --samples_we_have;
            }
        }
    }
}

void op_reset(void *vop) {
    static_cast<op_t*>(vop)->stats = query_stats_t();
}

void op_destroy(void *vop) {
    delete static_cast<op_t*>(vop);
}

void py_initialize_mysql_table(const char *server_str, int max_key, int max_value) {
    initialize_mysql_table(server_str, max_key, max_value);
}
