#ifndef __CONCURRENT_IO_DEPENDENCIES_HPP__
#define __CONCURRENT_IO_DEPENDENCIES_HPP__

#include <set>
#include <map>
#include <vector>
#include <libaio.h>

/*
* TODO!
*/
class concurrent_io_dependencies_t {
public:
    bool is_conflicting(iocb *request);

    void register_active_request(iocb *request);
    void unregister_request(iocb *request);

private:
    // Map is from file descriptor to a set of block offsets which have an active writer
    std::map<int, std::set<size_t> > active_writers;
    // Map is from file descriptor to a map from block offsets to the number of active readers
    std::map<int, std::map<size_t, unsigned int> > active_readers;

    std::vector<size_t> calculate_overlapping_blocks(iocb* request) const;
};

#endif /* __CONCURRENT_IO_DEPENDENCIES_HPP__ */
