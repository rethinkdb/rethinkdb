
#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <pthread.h>
#include "protocol.hpp"
#include "sqlite_protocol.hpp"
#include <stdint.h>
#include "sqlite3.h"
#include "assert.h"

#define update_salt 6830

using namespace std;

/* Information shared between clients */
struct shared_t {
public:
    shared_t(config_t *_config)
        : config(_config),
          qps_offset(0), latencies_offset(0),
          qps_fd(NULL), latencies_fd(NULL),
          last_qps(0), n_op(1), n_tick(1), n_ops_so_far(0)
        {
            pthread_mutex_init(&mutex, NULL);

            if(config->qps_file[0] != 0) {
                qps_fd = fopen(config->qps_file, "wa");
            }
            if(config->latency_file[0] != 0) {
                latencies_fd = fopen(config->latency_file, "wa");
            }

            value_buf = new char[config->values.max];
            memset((void *) value_buf, 'A', config->values.max);
        }

    ~shared_t() {
        if(qps_fd) {
            fwrite(qps, 1, qps_offset, qps_fd);
            fclose(qps_fd);
        }
        if(latencies_fd) {
            fwrite(latencies, 1, latencies_offset, latencies_fd);
            fclose(latencies_fd);
        }

        pthread_mutex_destroy(&mutex);
        
        delete value_buf;
    }

    void push_qps(int _qps, int tick) {
        if(!qps_fd && !latencies_fd)
            return;

        lock();

        // Collect qps info from all clients before attempting to print
        if(config->clients > 1) {
            int qps_count = 0, agg_qps = 0;
            map<int, pair<int, int> >::iterator op = qps_map.find(tick);
            if(op != qps_map.end()) {
                qps_count = op->second.first;
                agg_qps = op->second.second;
            }

            qps_count += 1;
            agg_qps += _qps;

            if(qps_count == config->clients) {
                _qps = agg_qps;
                qps_map.erase(op);
            } else {
                qps_map[tick] = pair<int, int>(qps_count, agg_qps);
                unlock();
                return;
            }
        }

        last_qps = _qps;

        if(!qps_fd) {
            unlock();
            return;
        }

        int _off = snprintf(qps + qps_offset, sizeof(qps) - qps_offset, "%d\t\t%d\n", n_tick, _qps);
        if(_off >= sizeof(qps) - qps_offset) {
            // Couldn't write everything, flush
            fwrite(qps, 1, qps_offset, qps_fd);

            // Write again
            qps_offset = 0;
            _off = snprintf(qps + qps_offset, sizeof(qps) - qps_offset, "%d\t\t%d\n", n_tick, _qps);
        }
        qps_offset += _off;

        n_tick++;

        unlock();
    }

    void push_latency(float latency) {
        lock();
        n_ops_so_far++;
        unlock();

        /*
        if(n_ops_so_far % 200000 == 0)
            printf("%ld\n", n_ops_so_far);
        */
        if(!latencies_fd || last_qps == 0)
            return;

        // We cannot possibly write every latency because that stalls
        // the client, so we want to scale that by the number of qps
        // (we'll sample latencies for roughly N random ops every
        // second).
        const int samples_per_second = 20;
        if(rand() % (last_qps / samples_per_second) != 0) {
            return;
        }

        lock();

        int _off = snprintf(latencies + latencies_offset, sizeof(latencies) - latencies_offset, "%ld\t\t%.2f\n", n_op, latency);
        if(_off >= sizeof(latencies) - latencies_offset) {
            // Couldn't write everything, flush
            fwrite(latencies, 1, latencies_offset, latencies_fd);

            // Write again
            latencies_offset = 0;
            _off = snprintf(latencies + latencies_offset, sizeof(latencies) - latencies_offset, "%ld\t\t%.2f\n", n_op, latency);
        }
        latencies_offset += _off;

        n_op++;

        unlock();
    }

private:
    config_t *config;
    map<int, pair<int, int> > qps_map;
    char qps[40960], latencies[40960];
    int qps_offset, latencies_offset;
    FILE *qps_fd, *latencies_fd;
    pthread_mutex_t mutex;
    int last_qps;

    long n_op;
    int n_tick;
    long n_ops_so_far;

public:
    // We have one value shared among all the threads.
    char *value_buf;

private:
    void lock() {
        pthread_mutex_lock(&mutex);
    }
    void unlock() {
        pthread_mutex_unlock(&mutex);
    }
};

/* Communication structure for main thread and clients */
struct client_data_t {
    config_t *config;
    server_t *server;
    shared_t *shared;
    protocol_t *proto;
    sqlite_protocol_t *sqlite;
    int id;
    int min_seed, max_seed;
};

/* The function that does the work */
void* run_client(void* data) {
    // Grab the config
    client_data_t *client_data = (client_data_t*)data;
    config_t *config = client_data->config;
    server_t *server = client_data->server;
    shared_t *shared = client_data->shared;
    protocol_t *proto = client_data->proto;
    sqlite_protocol_t *sqlite = client_data->sqlite;
    if(sqlite)
        sqlite->set_id(client_data->id);

    // Perform the ops
    ticks_t last_time = get_ticks(), start_time = last_time, last_qps_time = last_time, now_time;
    int qps = 0, tick = 0;
    int total_queries = 0;
    int total_inserts = 0;
    int total_deletes = 0;
    bool keep_running = true;
    while(keep_running) {
        // Generate the command
        load_t::load_op_t cmd = config->load.toss((config->batch_factor.min + config->batch_factor.max) / 2.0f);

        payload_t op_keys[config->batch_factor.max];
        char key_space[config->keys.max * config->batch_factor.max];

        for (int i = 0; i < config->batch_factor.max; i++)
            op_keys[i].first = key_space + (config->keys.max * i);

        payload_t op_vals[config->batch_factor.max];

        for (int i = 0; i < config->batch_factor.max; i++)
            op_vals[i].first = shared->value_buf;

        char val_verifcation_buffer[MAX_VALUE_SIZE];
        char *old_val_buffer;

        int j, k, l; // because we can't declare in the loop
        int count;
        int keyn; //same deal
        uint64_t id_salt = client_data->id;
        id_salt += id_salt << 40;

        // TODO: If an workload contains contains no inserts and there are no keys available for a particular client (and the duration is specified in q/i), it'll just loop forever.
        switch(cmd) {
        case load_t::delete_op:
            if (client_data->min_seed == client_data->max_seed)
                break;

            config->keys.toss(op_keys, client_data->min_seed ^ id_salt);

            // Delete it from the server
            proto->remove(op_keys->first, op_keys->second);
            if (sqlite && (client_data->min_seed % RELIABILITY) == 0)
                sqlite->remove(op_keys->first, op_keys->second);

            client_data->min_seed++;

            qps++;
            total_queries++;
            total_deletes++;
            break;

        case load_t::update_op:
            // Find the key and generate the payload
            if (client_data->min_seed == client_data->max_seed)
                break;

            keyn = random(client_data->min_seed, client_data->max_seed);

            config->keys.toss(op_keys, keyn ^ id_salt);
            op_vals[0].second = seeded_random(config->values.min, config->values.max, client_data->max_seed ^ id_salt ^ update_salt);

            // Send it to server
            proto->update(op_keys->first, op_keys->second, op_vals[0].first, op_vals[0].second);

            if (sqlite && (keyn % RELIABILITY) == 0)
                sqlite->update(op_keys->first, op_keys->second, op_vals[0].first, op_vals[0].second);

            // Free the value
            qps++;
            total_queries++;
            break;

        case load_t::insert_op:
            // Generate the payload
            config->keys.toss(op_keys, client_data->max_seed ^ id_salt);
            op_vals[0].second = seeded_random(config->values.min, config->values.max, client_data->max_seed ^ id_salt);

            // Send it to server
            proto->insert(op_keys->first, op_keys->second, op_vals[0].first, op_vals[0].second);

            if (sqlite && (client_data->max_seed % RELIABILITY) == 0)
                sqlite->insert(op_keys->first, op_keys->second, op_vals[0].first, op_vals[0].second);

            client_data->max_seed++;

            // Free the value and save the key
            qps++;
            total_queries++;
            total_inserts++;
            break;

        case load_t::read_op:
            // Find the key
            if(client_data->min_seed == client_data->max_seed)
                break;
            j = random(config->batch_factor.min, config->batch_factor.max);
            j = std::min(j, client_data->max_seed - client_data->min_seed);
            l = random(client_data->min_seed, client_data->max_seed - 1);
            for (k = 0; k < j; k++) {
                config->keys.toss(&op_keys[k], l ^ id_salt);
                l++;
                if(l >= client_data->max_seed)
                    l = client_data->min_seed;

            }
            // Read it from the server
            proto->read(&op_keys[0], j);

            qps += j;
            total_queries += j;
            break;
        case load_t::append_op:
            //TODO, this doesn't check if we'll be making the value too big. Gotta check for that.
            //Find the key
            if (client_data->min_seed == client_data->max_seed)
                break;

            keyn = random(client_data->min_seed, client_data->max_seed);

            config->keys.toss(op_keys, keyn ^ id_salt);
            op_vals[0].second = seeded_random(config->values.min, config->values.max, client_data->max_seed ^ id_salt);

            proto->append(op_keys->first, op_keys->second, op_vals->first, op_vals->second);
            if (sqlite && (keyn % RELIABILITY) == 0)
                sqlite->append(op_keys->first, op_keys->second, op_vals->first, op_vals->second);

            qps++;
            total_queries++;
            break;

        case load_t::prepend_op:
            //Find the key
            if (client_data->min_seed == client_data->max_seed)
                break;

            keyn = random(client_data->min_seed, client_data->max_seed);

            config->keys.toss(op_keys, keyn ^ id_salt);
            op_vals[0].second = seeded_random(config->values.min, config->values.max, client_data->max_seed ^ id_salt);

            proto->prepend(op_keys->first, op_keys->second, op_vals->first, op_vals->second);
            if (sqlite && (keyn % RELIABILITY) == 0)
                sqlite->prepend(op_keys->first, op_keys->second, op_vals->first, op_vals->second);

            qps++;
            total_queries++;
            break;
        case load_t::verify_op:
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
            while (sqlite->dump_next(op_keys, op_vals)) {
                proto->read(op_keys, 1, op_vals);
            }
            sqlite->dump_end();

            op_vals[0].first = old_val_buffer;

            qps++;
            total_queries++;
            break;
        default:
            fprintf(stderr, "Uknown operation\n");
            exit(-1);
        };
        now_time = get_ticks();

        // Deal with individual op latency
        ticks_t latency = now_time - last_time;
        shared->push_latency(ticks_to_us(latency));
        last_time = now_time;

        // Deal with QPS
        if(ticks_to_secs(now_time - last_qps_time) >= 1.0f) {
            shared->push_qps(qps, tick);

            last_qps_time = now_time;
            qps = 0;
            tick++;
        }

        // See if we should keep running
        if (config->duration.duration != -1) {
            switch(config->duration.units) {
            case duration_t::queries_t:
                keep_running = total_queries < config->duration.duration / config->clients;
                break;
            case duration_t::seconds_t:
                keep_running = ticks_to_secs(now_time - start_time) < config->duration.duration;
                break;
            case duration_t::inserts_t:
                keep_running = total_inserts - total_deletes < config->duration.duration / config->clients;
                break;
            default:
                fprintf(stderr, "Unknown duration unit\n");
                exit(-1);
            }
        }
    }
}

#endif // __CLIENT_HPP__

