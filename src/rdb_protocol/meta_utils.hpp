#ifndef RDB_PROTOCOL_META_UTILS_HPP_
#define RDB_PROTOCOL_META_UTILS_HPP_

#include <string>

#include "clustering/administration/metadata.hpp"

void wait_for_rdb_table_readiness(namespace_repo_t<rdb_protocol_t> *ns_repo, namespace_id_t namespace_id, signal_t *interruptor, boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata) THROWS_ONLY(interrupted_exc_t);

namespace ql {

static const char *valid_char_msg = name_string_t::valid_char_msg;

template<class T>
static void meta_check(metadata_search_status_t status, metadata_search_status_t want,
                       const std::string &operation, T *caller) {
    if (status != want) {
        const char *msg;
        switch (status) {
        case METADATA_SUCCESS: msg = "Entry already exists."; break;
        case METADATA_ERR_NONE: msg = "No entry with that name."; break;
        case METADATA_ERR_MULTIPLE: msg = "Multiple entries with that name."; break;
        case METADATA_CONFLICT: msg = "Entry with that name is in conflict."; break;
        default:
            unreachable();
        }
        rfail_target(caller, "Error during operation `%s`: %s", operation.c_str(), msg);
    }
}

template<class T, class U, class V>
static uuid_u meta_get_uuid(T *searcher, const U &predicate,
                            const std::string &operation, V *caller) {
    metadata_search_status_t status;
    typename T::iterator entry = searcher->find_uniq(predicate, &status);
    meta_check(status, METADATA_SUCCESS, operation, caller);
    return entry->first;
}

} // namespace ql
#endif // RDB_PROTOCOL_META_UTILS_HPP_
