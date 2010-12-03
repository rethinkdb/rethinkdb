
#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <pthread.h>
#include "protocol.hpp"
#include <stdint.h>

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
    int id;
    int min_seed, max_seed;

    /* \brief counters are used for to keep track of which keys we've done an
     * operation on, they allow you to do an operation on every nth key */
    class counter {
    private:
        int val, delta;
    public:
        counter()
            :delta(1), val(0)
        {}
        counter(int delta, int val = 0) 
            :delta(delta), val(val)
        {}
        void init(int _delta, int _val = 0) {
            this->delta = _delta;
            this->val = _val;
        }
        /* \brief get the next value to apply the operation to min and max must
         * be monotonically increasing on successive calls to this function
         * returns -1 if a suitable values does not exist*/
        int next_val(int min, int max) {
            int res;
            if (min <= val && val < max) {
                res = val;
                val += delta;
            } else {
                int remainder = min % delta;

                if (remainder == 0) {
                    res = min;
                    val = min + delta;
                } else if (min + delta - remainder < max) {
                    res = min + delta-remainder;
                    val = res + delta;
                } else {
                    return -1;
                }

                return res;
            }
        }

        /* \brief whether a value was ever returned by next_val */
        bool was_returned(int v) {
            return (v < val && v % delta == 0);
        }
    };

    counter update_c, append_c, prepend_c;
};

inline uint64_t id_salt(client_data_t *client_data) {
    uint64_t res = client_data->id;
    res += res << 40;
    return res;
}

void set_val(client_data_t *cd, payload_t *val, int n) {
    cd->config->keys.toss(val, n);
}

#define update_salt 1234567
void update_val(client_data_t *cd, payload_t *val, int n) {
    cd->config->keys.toss(val, n ^ update_salt ^ id_salt(cd));
}

#define append_salt 545454
void append_val(client_data_t *cd, payload_t *val, int n) {
    cd->config->keys.toss(val, n ^ append_salt ^ id_salt(cd));
}

#define prepend_salt 9876543
void prepend_val(client_data_t *cd, payload_t *val, int n) {
    cd->config->keys.toss(val, n ^ prepend_salt ^ id_salt(cd));
}

int in_db_val(client_data_t *cd, payload_t *val, int n) {
    if (n >= cd->max_seed || n < cd->min_seed) {
        return -1;
    }

    if (!cd->update_c.was_returned(n)) {
        set_val(cd, val, n);
        char other_data[MAX_VAL_SIZE];
        payload_t other;
        other.first = other_data;
        other.second = 0;
        if (cd->append_c.was_returned(n)) {
            append_val(cd, &other, n);
            append(val, &other);
        }
        if (cd->prepend_c.was_returned(n)) {
            prepend_val(cd, &other, n);
            prepend(val, &other);
        }
    } else {
        update_val(cd, val, n);
    }

    return 0;
}

/* The function that does the work */
void* run_client(void* data) {
    // Grab the config
    client_data_t *client_data = (client_data_t*)data;
    config_t *config = client_data->config;
    server_t *server = client_data->server;
    shared_t *shared = client_data->shared;
    protocol_t *proto = client_data->proto;

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

        int j, k, l; // because we can't declare in the loop
        uint64_t id_salt = client_data->id;
        id_salt += id_salt << 40;

        // TODO: If an workload contains contains no inserts and there are no keys available for a particular client (and the duration is specified in q/i), it'll just loop forever.
        switch(cmd) {
        case load_t::delete_op:
            if (client_data->min_seed == client_data->max_seed)
                break;

            config->keys.toss(op_keys, client_data->min_seed ^ id_salt);
            client_data->min_seed++;

            // Delete it from the server
            proto->remove(op_keys->first, op_keys->second);

            //clean up the memory
            qps++;
            total_queries++;
            total_deletes++;
            break;

        case load_t::update_op:
            // Find the key and generate the payload
            if (client_data->min_seed == client_data->max_seed)
                break;

            config->keys.toss(op_keys, random(client_data->min_seed, client_data->max_seed) ^ id_salt);

            op_vals[0].second = random(config->values.min, config->values.max);

            // Send it to server
            proto->update(op_keys->first, op_keys->second, op_vals[0].first, op_vals[0].second);
            // Free the value
            qps++;
            total_queries++;
            break;

        case load_t::insert_op:
            // Generate the payload
            config->keys.toss(op_keys, client_data->max_seed ^ id_salt);
            op_vals[0].second = seeded_random(config->values.min, config->values.max, client_data->max_seed ^ id_salt);

            client_data->max_seed++;
            // Send it to server
            proto->insert(op_keys->first, op_keys->second, op_vals[0].first, op_vals[0].second);
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
                op_vals[k].second = seeded_random(config->values.min, config->values.max, l ^ id_salt);
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
            break;
        case load_t::prepend_op:
            break;
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

