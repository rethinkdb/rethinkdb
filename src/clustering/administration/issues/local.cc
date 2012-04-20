#include "clustering/administration/issues/local.hpp"

#include "clustering/administration/persist.hpp"

int deserialize(read_stream_t *s, local_issue_t **issue_ptr) {
    bool exists;
    int res = deserialize(s, &exists);
    if (res) { return res; }
    if (!exists) {
        *issue_ptr = NULL;
        return 0;
    }

    int8_t code;
    res = deserialize(s, &code);
    if (res) { return res; }

    // The only subclass is persistence_issue_t.
    if (code != local_issue_t::PERSISTENCE_ISSUE_CODE) { return -3; }

    std::string desc;
    res = deserialize(s, &desc);
    if (res) { return res; }

    *issue_ptr = new metadata_persistence::persistence_issue_t(desc);

    return 0;
}
