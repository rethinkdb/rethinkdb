template<class payload_t>
accounting_diskmgr_t<payload_t>::accounting_diskmgr_t(int target_num_active) :
    total_priority(0), last_dequeue_pointer(0),
    target_num_active(target_num_active), num_active(0) { }

template<class payload_t>
accounting_diskmgr_t<payload_t>::~accounting_diskmgr_t() {
    rassert(num_active == 0);
    rassert(active_accounts.empty());
}

template<class payload_t>
void accounting_diskmgr_t<payload_t>::submit(action_t *a) {

    rassert(a->account->parent == this);

    if (a->account->queue.empty()) activate_account(a->account);
    a->account->queue.push_back(a);
    pump();
}

template<class payload_t>
void accounting_diskmgr_t<payload_t>::done(payload_t *p) {
    action_t *a = static_cast<action_t *>(p);
    num_active--;
    pump();
    done_fun(a);
}

template<class payload_t>
void accounting_diskmgr_t<payload_t>::activate_account(account_t *a) {
    active_accounts.push_back(a);
    total_priority += a->priority;
}

template<class payload_t>
void accounting_diskmgr_t<payload_t>::deactivate_account(account_t *a) {
    active_accounts.remove(a);
    total_priority -= a->priority;
}

template<class payload_t>
void accounting_diskmgr_t<payload_t>::pump() {
    while (!active_accounts.empty() && num_active < target_num_active) {

        /* Choose which account to run the next operation from */
        last_dequeue_pointer %= total_priority;
        typename intrusive_list_t<account_t>::iterator account_it;
        account_it = active_accounts.begin();
        int i = 0;
        while (true) {
            i += (*account_it).priority;
            if (i >= last_dequeue_pointer) break;
            account_it++;
        }
        account_t *next_account = &(*account_it);
        last_dequeue_pointer++;

        /* Take an operation from the account's queue */
        action_t *a = next_account->queue.head();
        next_account->queue.remove(a);
        if (next_account->queue.empty()) deactivate_account(next_account);

        /* Run the operation */
        num_active++;
        submit_fun(a);
    }
}
