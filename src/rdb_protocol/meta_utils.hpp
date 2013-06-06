#ifndef RDB_PROTOCOL_META_UTILS_HPP_
#define RDB_PROTOCOL_META_UTILS_HPP_

#include <string>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"

void wait_for_rdb_table_readiness(
    base_namespace_repo_t<rdb_protocol_t> *ns_repo,
    namespace_id_t namespace_id,
    signal_t *interruptor,
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        semilattice_metadata) THROWS_ONLY(interrupted_exc_t);

namespace ql {

template<class T, class U, class V>
uuid_u meta_get_uuid(T *searcher, const U &predicate,
                     const std::string &message, V *caller) {
    metadata_search_status_t status;
    typename T::iterator entry = searcher->find_uniq(predicate, &status);
    rcheck_target(caller, base_exc_t::GENERIC,
                  status == METADATA_SUCCESS, message);
    return entry->first;
}

} // namespace ql
#endif // RDB_PROTOCOL_META_UTILS_HPP_
