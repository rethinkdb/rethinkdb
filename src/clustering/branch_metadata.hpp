

template<class protocol_t>
class branch_metadata_t {

public:
    /* The initial state of the branch is composed of these regions of these
    versions. */

    std::vector<std::pair<typename protocol_t::region_t, version_t> > parents;
};

