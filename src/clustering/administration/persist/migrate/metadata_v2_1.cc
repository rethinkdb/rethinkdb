// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "clustering/administration/persist/migrate/metadata_v2_1.hpp"

namespace metadata_v2_1 {

metadata_file_t::key_t<metadata_v1_16::auth_semilattice_metadata_t>
mdkey_auth_semilattices() {
    return metadata_file_t::key_t<metadata_v1_16::auth_semilattice_metadata_t>
        ("auth_semilattice");
}

} // namespace metadata_v2_1

