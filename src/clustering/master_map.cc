#include "clustering/master_map.hpp"
#include "errors.hpp"

int master_map_t::master_hasher_t::get_bucket(store_key_t key) {
    return hasher.hash(key) % (1 << log_buckets);
}

master_map_t::set_store_txn_t master_map_t::get_master(store_key_t key) {
    int bucket = master_hasher.get_bucket(key) % (1 << master_hasher.log_buckets);
    int depth = 0;
    while (depth <= master_hasher.log_buckets) {
        int step = 1 << (master_hasher.log_buckets - depth);
        int modulos = 1 << master_hasher.log_buckets;
        for (int i = 0; i < (1 << depth); i++) {
            if (inner_map.find((bucket + (i * step)) % modulos) != inner_map.end())
                return set_store_txn_t(inner_map[(bucket + (i * step)) % modulos], this, bucket);
        }
    }
    crash("There are currently no masters, in the future this won't be a crash\n");
    set_store_txn_t compiler_happiness_enhancer;
    return compiler_happiness_enhancer;
}

typedef storage_map_t::rh_iterator rh_iterator;
typedef storage_map_t::iterator storage_iterator;

rh_iterator storage_map_t::redundant_hasher_t::get_buckets(store_key_t key) {
    hash_t raw = hasher.hash(key);
    int modulos = 1 << (log_buckets - log_redundancy);
    return storage_map_t::redundant_hasher_t::iterator(raw % modulos, modulos, log_buckets);
}

bool rh_iterator::operator==(rh_iterator const &other) {
    return ((val == other.val && modulos == other.modulos && log_buckets == other.log_buckets) ||
            (val >= (1 << log_buckets) && other.end) ||
            (other.val >= (1 << other.log_buckets) && end) ||
            (end && other.end));
} 

bool rh_iterator::operator!=(rh_iterator const &other) {
    return (!(*this == other));
}

int rh_iterator::operator*() const { 
    guarantee(!end && val < (1 << log_buckets), "Trying to dereference end");
    return val; 
}

rh_iterator rh_iterator::operator++() {
    val += modulos; 
    return *this;
}

rh_iterator rh_iterator::operator++(int) {
    val += modulos; 
    return *this; 
}

storage_iterator::iterator(std::map<int, std::pair<set_store_t*, get_store_t *> > *inner_map, redundant_hasher_t *rh, redundant_hasher_t::iterator hasher_iterator) 
    : inner_map(inner_map), 
      rh(rh),
      hasher_iterator(hasher_iterator)
{
    while (hasher_iterator != rh->end() && inner_map->find(*hasher_iterator) != inner_map->end())
        hasher_iterator++;
}

std::pair<set_store_t *, get_store_t *> storage_iterator::operator*() const {
    guarantee(inner_map->find(*hasher_iterator) != inner_map->end(), "Trying to dereference a map that doesn't exist. This means that jdoliner messed up this iterator");
    //logINF("Yielding peer %d in indirection operator.\n", *hasher_iterator);
    return (*inner_map)[*hasher_iterator];
}
storage_iterator storage_iterator::operator++() { 
    hasher_iterator++; 

    while (hasher_iterator != rh->end() && inner_map->find(*hasher_iterator) != inner_map->end())
        hasher_iterator++;

    return *this;
}

storage_iterator storage_iterator::operator++(int) { return this->operator++(); }

bool storage_iterator::operator==(storage_iterator const &other) { return (hasher_iterator == other.hasher_iterator); }

bool storage_iterator::operator!=(storage_iterator const &other) { return !(*this == other); }

storage_iterator storage_map_t::get_storage(store_key_t key) { 
    return storage_iterator(&inner_map, &rh, rh.get_buckets(key));
}

storage_iterator storage_map_t::end() {
    return storage_iterator(&inner_map, &rh, rh.end());
}
