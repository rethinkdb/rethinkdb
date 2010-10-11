
#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <pthread.h>
#include "protocol.hpp"

using namespace std;

/* Information shared between clients */
struct shared_t {
public:
    typedef protocol_t*(*protocol_factory_t)(void);
    
public:
    shared_t(config_t *_config, protocol_factory_t _protocol_factory)
        : config(_config),
          qps_offset(0), latencies_offset(0),
          qps_fd(NULL), latencies_fd(NULL),
          protocol_factory(_protocol_factory)
        {
            pthread_mutex_init(&mutex, NULL);
            
            if(config->qps_file[0] != 0) {
                qps_fd = fopen(config->qps_file, "wa");
            }
            if(config->latency_file[0] != 0) {
                latencies_fd = fopen(config->latency_file, "wa");
            }
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
    }
    
    void push_qps(int _qps, int tick) {
        if(!qps_fd)
            return;
        
        lock();

        // Collect qps info from all clients before attempting to print
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
            
        int _off = snprintf(qps + qps_offset, sizeof(qps) - qps_offset, "%d\n", _qps);
        if(_off >= sizeof(qps) - qps_offset) {
            // Couldn't write everything, flush
            fwrite(qps, 1, qps_offset, qps_fd);
            
            // Write again
            qps_offset = 0;
            _off = snprintf(qps + qps_offset, sizeof(qps) - qps_offset, "%d\n", _qps);
        }
        qps_offset += _off;
        
        unlock();
    }

    void push_latency(float latency) {
        if(!latencies_fd)
            return;

        lock();
        
        int _off = snprintf(latencies + latencies_offset, sizeof(latencies) - latencies_offset, "%.2f\n", latency);
        if(_off >= sizeof(latencies) - latencies_offset) {
            // Couldn't write everything, flush
            fwrite(latencies, 1, latencies_offset, latencies_fd);
            
            // Write again
            latencies_offset = 0;
            _off = snprintf(latencies + latencies_offset, sizeof(latencies) - latencies_offset, "%.2f\n", latency);
        }
        latencies_offset += _off;

        unlock();
    }

public:
    protocol_factory_t protocol_factory;
    
private:
    config_t *config;
    map<int, pair<int, int> > qps_map;
    char qps[40960], latencies[40960];
    int qps_offset, latencies_offset;
    FILE *qps_fd, *latencies_fd;
    pthread_mutex_t mutex;

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
    shared_t *shared;
};

/* The function that does the work */
void* run_client(void* data) {
    // Grab the config
    client_data_t *client_data = (client_data_t*)data;
    config_t *config = client_data->config;
    shared_t *shared = client_data->shared;
    shared_t::protocol_factory_t pf = shared->protocol_factory;
    protocol_t *proto = (*pf)();
    
    // Connect to the server
    proto->connect(config->host, config->port);

    // Store the keys so we can run updates and deletes.
    vector<payload_t> keys;

    // Perform the ops
    ticks_t last_time = get_ticks(), last_qps_time = last_time, now_time;
    int qps = 0, tick = 0;
    for(int i = 0; i < config->duration / config->clients; i++) {
        // Generate the command
        load_t::load_op_t cmd = config->load.toss();

        int _val;
        payload_t key, value;
        
        switch(cmd) {
        case load_t::delete_op:
            // Find the key
            if(keys.empty())
                break;
            _val = random(0, keys.size() - 1);
            key = keys[_val];
            // Delete it from the server
            proto->remove(key.first, key.second);
            // Our own bookkeeping
            free(key.first);
            keys[_val] = keys[keys.size() - 1];
            keys.erase(keys.begin() + _val);
            break;
            
        case load_t::update_op:
            // Find the key and generate the payload
            if(keys.empty())
                break;
            key = keys[random(0, keys.size() - 1)];
            config->values.toss(&value);
            // Send it to server
            proto->update(key.first, key.second, value.first, value.second);
            // Free the value
            free(value.first);
            break;
            
        case load_t::insert_op:
            // Generate the payload
            config->keys.toss(&key);
            config->values.toss(&value);
            // Send it to server
            proto->insert(key.first, key.second, value.first, value.second);
            // Free the value and save the key
            free(value.first);
            keys.push_back(key);
            break;
            
        case load_t::read_op:
            // Find the key
            if(keys.empty())
                break;
            key = keys[random(0, keys.size() - 1)];
            // Read it from the server
            proto->read(key.first, key.second);
            break;
        };
        now_time = get_ticks();
        qps++;

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
    }

    delete proto;
    
    // Free all the keys
    for(vector<payload_t>::iterator i = keys.begin(); i != keys.end(); i++) {
        free(i->first);
    }
}

#endif // __CLIENT_HPP__

