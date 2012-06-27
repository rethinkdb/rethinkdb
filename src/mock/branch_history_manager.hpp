#ifndef MOCK_BRANCH_HISTORY_MANAGER_HPP_
#define MOCK_BRANCH_HISTORY_MANAGER_HPP_

#include "clustering/immediate_consistency/branch/history.hpp"

namespace mock {

template <class protocol_t>
class in_memory_branch_history_manager_t : public branch_history_manager_t<protocol_t> {
public:
    branch_birth_certificate_t<protocol_t> get_branch(branch_id_t branch) THROWS_NOTHING;
    void create_branch(branch_id_t branch_id, const branch_birth_certificate_t<protocol_t> &bc, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
    void export_branch_history(branch_id_t branch, branch_history_t<protocol_t> *out) THROWS_NOTHING;
    void import_branch_history(const branch_history_t<protocol_t> &new_records, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

private:
    branch_history_t<protocol_t> bh;
};

}   /* namespace mock */

#include "mock/branch_history_manager.tcc"

#endif /* MOCK_BRANCH_HISTORY_MANAGER_HPP_ */
