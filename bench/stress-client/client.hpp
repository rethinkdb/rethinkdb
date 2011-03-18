
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
    int inserts_minus_deletes;
    ticks_t worst_latency;
    reservoir_sample_t<ticks_t> latency_samples;

    query_stats_t() :
        queries(0), inserts_minus_deletes(0), worst_latency(0) { }

    void push(ticks_t latency, int imd, bool enable_latency_samples, int batch_count) {
        queries += batch_count;
        inserts_minus_deletes += imd;
        worst_latency = std::max(worst_latency, latency);
        if (enable_latency_samples) latency_samples.push(latency);
    }

    void aggregate(const query_stats_t *other) {
        queries += other->queries;
        inserts_minus_deletes += other->inserts_minus_deletes;
        worst_latency = std::max(worst_latency, other->worst_latency);
        latency_samples += other->latency_samples;
    }
};

/* Communication structure for main thread and clients */
struct client_data_t {
    load_t *load;
    server_t *server;
    protocol_t *proto;
    sqlite_protocol_t *sqlite;
    int id, num_clients;
    int min_seed, max_seed;

    // This spinlock governs all the communications variables between the client and main threads.
    spinlock_t spinlock;

    // Main thread sets this to false in order to stop the client
    bool keep_running;

    bool enable_latency_samples;

    // Stat reporting variables. These are updated by the client thread and read & reset by the
    // main thread.
    query_stats_t stats;
};

/* The function that does the work */
void* run_client(void* data) {
    // Grab the config
    client_data_t *client_data = (client_data_t*)data;
    load_t *load = client_data->load ;
    server_t *server = client_data->server;
    protocol_t *proto = client_data->proto;
    sqlite_protocol_t *sqlite = client_data->sqlite;
    if(sqlite)
        sqlite->set_id(client_data->id);

    bool enable_latency_samples = client_data->enable_latency_samples;

    const size_t per_key_size = load->keys.calculate_max_length(client_data->num_clients - 1);

    char *buffer_of_As = new char[load->values.max];
    memset((void *) buffer_of_As, 'A', load->values.max);

    client_data->spinlock.lock();
    rnd_gen_t _rnd = xrandom_create(load->distr, load->mu);
    while(client_data->keep_running) {
        client_data->spinlock.unlock();

        // Generate the command
        op_ratios_t::op_t cmd = load->op_ratios.toss((load->batch_factor.min + load->batch_factor.max) / 2.0f);

        payload_t op_keys[load->batch_factor.max];
        char key_space[per_key_size * load->batch_factor.max];

        for (int i = 0; i < load->batch_factor.max; i++)
            op_keys[i].first = key_space + (per_key_size * i);

        payload_t op_vals[load->batch_factor.max];

        for (int i = 0; i < load->batch_factor.max; i++)
            op_vals[i].first = buffer_of_As;

        char val_verifcation_buffer[MAX_VALUE_SIZE];
        char *old_val_buffer;

        // The operation we run may or may not acquire the spinlock itself. If it does it sets
        // have_lock to true.
        bool have_lock = false;

        uint64_t id_salt = client_data->id;
        id_salt += id_salt << 32;
        try {
            switch(cmd) {
            case op_ratios_t::delete_op: {
                if (client_data->min_seed == client_data->max_seed)
                    break;

                load->keys.toss(op_keys, client_data->min_seed ^ id_salt, client_data->id, client_data->num_clients - 1);

                // Delete it from the server
                ticks_t start_time = get_ticks();
                proto->remove(op_keys->first, op_keys->second);
                ticks_t end_time = get_ticks();

                if (sqlite && (client_data->min_seed % RELIABILITY) == 0)
                    sqlite->remove(op_keys->first, op_keys->second);

                client_data->min_seed++;

                client_data->spinlock.lock();
                have_lock = true;
                client_data->stats.push(end_time - start_time, -1, enable_latency_samples, 1);
                break;
            }

            case op_ratios_t::update_op: {
                // Find the key and generate the payload
                if (client_data->min_seed == client_data->max_seed)
                    break;

                int keyn = xrandom(_rnd, client_data->min_seed, client_data->max_seed);

                load->keys.toss(op_keys, keyn ^ id_salt, client_data->id, client_data->num_clients - 1);
                op_vals[0].second = seeded_xrandom(load->values.min, load->values.max, client_data->max_seed ^ id_salt ^ update_salt);

                // Send it to server
                ticks_t start_time = get_ticks();
                proto->update(op_keys->first, op_keys->second, op_vals[0].first, op_vals[0].second);
                ticks_t end_time = get_ticks();

                if (sqlite && (keyn % RELIABILITY) == 0)
                    sqlite->update(op_keys->first, op_keys->second, op_vals[0].first, op_vals[0].second);

                client_data->spinlock.lock();
                have_lock = true;
                client_data->stats.push(end_time - start_time, 0, enable_latency_samples, 1);
                break;
            }

            case op_ratios_t::insert_op: {
                // Generate the payload
                load->keys.toss(op_keys, client_data->max_seed ^ id_salt, client_data->id, client_data->num_clients - 1);
                op_vals[0].second = seeded_xrandom(load->values.min, load->values.max, client_data->max_seed ^ id_salt);

                // Send it to server
                ticks_t start_time = get_ticks();
                proto->insert(op_keys->first, op_keys->second, op_vals[0].first, op_vals[0].second);
                ticks_t end_time = get_ticks();

                if (sqlite && (client_data->max_seed % RELIABILITY) == 0)
                    sqlite->insert(op_keys->first, op_keys->second, op_vals[0].first, op_vals[0].second);

                client_data->max_seed++;

                client_data->spinlock.lock();
                have_lock = true;
                client_data->stats.push(end_time - start_time, 1, enable_latency_samples, 1);
                break;
            }

            case op_ratios_t::read_op: {
                // Find the key
                if(client_data->min_seed == client_data->max_seed)
                    break;
                int j = xrandom(load->batch_factor.min, load->batch_factor.max);
                j = std::min(j, client_data->max_seed - client_data->min_seed);
                int l = xrandom(_rnd, client_data->min_seed, client_data->max_seed - 1);
                for (int k = 0; k < j; k++) {
                    load->keys.toss(&op_keys[k], l ^ id_salt, client_data->id, client_data->num_clients - 1);
                    l++;
                    if(l >= client_data->max_seed)
                        l = client_data->min_seed;

                }
                // Read it from the server
                ticks_t start_time = get_ticks();
                proto->read(&op_keys[0], j);
                ticks_t end_time = get_ticks();

                client_data->spinlock.lock();
                have_lock = true;
                client_data->stats.push(end_time - start_time, 0, enable_latency_samples, j);
                break;
            }

            case op_ratios_t::append_op: {
                //TODO, this doesn't check if we'll be making the value too big. Gotta check for that.
                //Find the key
                if (client_data->min_seed == client_data->max_seed)
                    break;

                int keyn = xrandom(_rnd, client_data->min_seed, client_data->max_seed);

                load->keys.toss(op_keys, keyn ^ id_salt, client_data->id, client_data->num_clients - 1);
                op_vals[0].second = seeded_xrandom(load->values.min, load->values.max, client_data->max_seed ^ id_salt);

                ticks_t start_time = get_ticks();
                proto->append(op_keys->first, op_keys->second, op_vals->first, op_vals->second);
                ticks_t end_time = get_ticks();

                if (sqlite && (keyn % RELIABILITY) == 0)
                    sqlite->append(op_keys->first, op_keys->second, op_vals->first, op_vals->second);

                client_data->spinlock.lock();
                have_lock = true;
                client_data->stats.push(end_time - start_time, 0, enable_latency_samples, 1);
                break;
            }

            case op_ratios_t::prepend_op: {
                //Find the key
                if (client_data->min_seed == client_data->max_seed)
                    break;

                int keyn = xrandom(_rnd, client_data->min_seed, client_data->max_seed);

                load->keys.toss(op_keys, keyn ^ id_salt, client_data->id, client_data->num_clients - 1);
                op_vals[0].second = seeded_xrandom(load->values.min, load->values.max, client_data->max_seed ^ id_salt);

                ticks_t start_time = get_ticks();
                proto->prepend(op_keys->first, op_keys->second, op_vals->first, op_vals->second);
                ticks_t end_time = get_ticks();

                if (sqlite && (keyn % RELIABILITY) == 0)
                    sqlite->prepend(op_keys->first, op_keys->second, op_vals->first, op_vals->second);

                client_data->spinlock.lock();
                have_lock = true;
                client_data->stats.push(end_time - start_time, 0, enable_latency_samples, 1);
                break;
            }

            case op_ratios_t::verify_op: {
                /* this is a very expensive operation it will first do a very
                 * expensive operation on the SQLITE reference db and then it will
                 * do several queries on the db that's being stressed (and only add
                 * 1 total query), it does not make sense to use this as part of a
                 * benchmarking run */
                
                // we can't do anything without a reference
                if (!sqlite)
                    break;

                /* this is hacky but whatever */
                old_val_buffer = op_vals[0].first;
                op_vals[0].first = val_verifcation_buffer;
                op_vals[0].second = 0;

                sqlite->dump_start();
                ticks_t start_time = get_ticks();
                while (sqlite->dump_next(op_keys, op_vals)) {
                    proto->read(op_keys, 1, op_vals);
                }
                ticks_t end_time = get_ticks();
                sqlite->dump_end();

                op_vals[0].first = old_val_buffer;

                client_data->spinlock.lock();
                have_lock = true;
                client_data->stats.push(end_time - start_time, 0, enable_latency_samples, 1);
                break;
            }

            default:
                fprintf(stderr, "Uknown operation\n");
                exit(-1);
            };
        } catch (const protocol_error_t& e) {
            fprintf(stderr, "Protocol error: %s\n", e.c_str());
            exit(-1);
        }

        if (!have_lock) {
            // We must acquire the lock before we continue the while loop, or there is a race
            // condition on client_data->keep_running
            client_data->spinlock.lock();
        }
    }

    client_data->spinlock.unlock();

    delete[] buffer_of_As;
}

#endif // __CLIENT_HPP__

