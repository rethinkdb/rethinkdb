#ifndef __CLUSTERING_BRANCH_HISTORY_HPP__
#define __CLUSTERING_BRANCH_HISTORY_HPP__

#include <vector>

#include "errors.hpp"
#include <boost/uuid/uuids.hpp>

#include "timestamps.hpp"

typedef boost::uuid::uuid branch_id_t;

template<class protocol_t>
class version_t {
public:

    class part_t {
    public:
        part_t(typename protocol_t::region_t r, branch_id_t b, state_timestamp_t t) :
            region(r), branch(b), timestamp(t)
        {
            validate();
        }

        static part_t initial_state(typename protocol_t::region_t r) {
            return part_t(r, boost::uuid::nil_generator()(), state_timestamp_t::zero());
        }

        void validate() const {
            if (branch.is_nil()) {
                rassert(timestamp == state_timestamp_t::zero());
            }
        }

        void validate(const std::map<branch_id_t, version_t> &branch_info) const {
            validate();
            if (!branch.is_nil()) {
                std::map<branch_id_t, version_t>::iterator it = branch_info.find(branch);
                if (it != branch_info.end()) {
                    
        }

        typename protocol_t::region_t region;
        branch_id_t branch;
        state_timestamp_t timestamp;
    };

    version_t() { }
    version_t(std::vector<part_t> parts) : parts(parts) {
        validate();
    }

    void validate() const {
        for (int i = 0; i < (int)parts.size(); i++) {
            parts[i].validate();
            for (int j = i + 1; j < (int)parts.size(); i++) {
                rassert(!parts[j].region.overlaps(parts[i].region));
            }
        }
    }

    void validate(

    std::vector<part_t> parts;
};

#endif /* __CLUSTERING_BRANCH_HISTORY_HPP__ */
