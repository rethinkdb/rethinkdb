// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/scope.hpp"

template <class T>
void variable_scope_t<T>::put_in_scope(const std::string &name, const T &t) {
    guarantee_T(t);
    guarantee(!scopes.empty());
    scopes.front()[name] = t;
}

template <class T>
T variable_scope_t<T>::get(const std::string &name) const {
    for (typename std::list<std::map<std::string, T> >::const_iterator it = scopes.begin();
         it != scopes.end();
         ++it) {
        typename std::map<std::string, T>::const_iterator jt = it->find(name);
        if (jt != it->end()) {
            return jt->second;
        }
    }

    unreachable("Variable not in scope, probably because the code fails to call is_in_scope().");
}

// Calling this only makes sense in the typechecker. All variables
// are guranteed by the typechecker to be present at runtime.
template <class T>
bool variable_scope_t<T>::is_in_scope(const std::string &name) const {
    for (typename std::list<std::map<std::string, T> >::const_iterator it = scopes.begin(); it != scopes.end(); ++it) {
        typename std::map<std::string, T>::const_iterator jt = it->find(name);
        if (jt != it->end()) {
            return true;
        }
    }
    return false;
}

template <class T>
void variable_scope_t<T>::push() {
    scopes.push_front(std::map<std::string, T>());
}

template <class T>
void variable_scope_t<T>::pop() {
    scopes.pop_front();
}

// TODO(rntz): find a better way to do this.
template <class T>
void variable_scope_t<T>::dump(std::vector<std::string> *argnames, std::vector<T> *argvals) const {
    std::set<std::string> seen;

    if (argnames) argnames->clear();
    argvals->clear();

    // Most recent scope is at front of deque, so we iterate in-order.
    for (typename std::list<std::map<std::string, T> >::const_iterator sit = scopes.begin(); sit != scopes.end(); ++sit) {
        for (typename std::map<std::string, T>::const_iterator it = sit->begin(); it != sit->end(); ++it) {
            // Earlier bindings take precedence over later ones.
            if (seen.count(it->first)) continue;

            seen.insert(it->first);
            if (argnames) argnames->push_back(it->first);
            argvals->push_back(it->second);
        }
    }
}

template <class T>
variable_scope_t<T>::new_scope_t::new_scope_t(variable_scope_t<T> *_parent)
    : parent(_parent)
{
    parent->push();
}

template <class T>
variable_scope_t<T>::new_scope_t::new_scope_t(variable_scope_t<T> *_parent, const std::string &name, const T &t)
    : parent(_parent)
{
    parent->push();
    parent->put_in_scope(name, t);
}

template <class T>
variable_scope_t<T>::new_scope_t::~new_scope_t() {
    parent->pop();
}

template class variable_scope_t<boost::shared_ptr<scoped_cJSON_t> >;

#include "rdb_protocol/serializable_environment.hpp"
template class variable_scope_t<query_language::term_info_t>;
