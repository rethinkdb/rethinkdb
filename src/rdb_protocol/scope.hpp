#ifndef RDB_PROTOCOL_SCOPE_HPP_
#define RDB_PROTOCOL_SCOPE_HPP_

template <class T>
class variable_scope_t {
public:
    void put_in_scope(const std::string &name, const T &t) {
        rassert(!scopes.empty());
        scopes.front()[name] = t;
    }

    T get(const std::string &name) {
        for (typename scopes_t::iterator it  = scopes.begin();
                                         it != scopes.end();
                                         ++it) {
            typename std::map<std::string, T>::iterator jt = it->find(name);
            if (jt != it->end()) {
                return jt->second;
            }
        }

        unreachable("Variable not in scope, probably because the code fails to call is_in_scope().");
    }

    // Calling this only makes sense in the typechecker. All variables
    // are guranteed by the typechecker to be present at runtime.
    bool is_in_scope(const std::string &name) {
        for (typename scopes_t::iterator it  = scopes.begin();
                                         it != scopes.end();
                                         ++it) {
            typename std::map<std::string, T>::iterator jt = it->find(name);
            if (jt != it->end()) {
                return true;
            }
        }
        return false;
    }

    void push() {
        scopes.push_front(std::map<std::string, T>());
    }

    void pop() {
        scopes.pop_front();
    }

    // TODO (rntz): find a better way to do this.
    void dump(std::vector<std::string> *argnames, std::vector<T> *argvals) {
        std::set<std::string> seen;

        if (argnames) argnames->clear();
        argvals->clear();

        // Most recent scope is at front of deque, so we iterate in-order.
        for (typename std::deque<std::map<std::string, T> >::iterator sit = scopes.begin(); sit != scopes.end(); ++sit) {
            for (typename std::map<std::string, T>::iterator it = sit->begin(); it != sit->end(); ++it) {
                // Earlier bindings take precedence over later ones.
                if (seen.count(it->first)) continue;

                seen.insert(it->first);
                if (argnames) argnames->push_back(it->first);
                argvals->push_back(it->second);
            }
        }
    }

    struct new_scope_t {
        explicit new_scope_t(variable_scope_t<T> *_parent)
            : parent(_parent)
        {
            parent->push();
        }
        ~new_scope_t() {
            parent->pop();
        }

        variable_scope_t<T> *parent;
    };
private:
    typedef std::deque<std::map<std::string, T> > scopes_t;
    scopes_t scopes;
};

/* an implicit_value_t allows for a specific implicit value to exist at certain
 * points in execution for example the argument to get attr is implicitly
 * defined to be the value of the row upon entering a filter,map etc.
 * implicit_value_t supports scopes for its values but does not allow looking
 * up values in any scope to the current one. */
template <class T>
class implicit_value_t {
public:
    implicit_value_t() {
        push();
    }

    void push() {
        scopes.push_front(boost::optional<T>());
    }

    void push(const T &t) {
        scopes.push_front(t);
    }

    void pop() {
        scopes.pop_front();
    }

    class impliciter_t {
    public:
        explicit impliciter_t(implicit_value_t *_parent)
            : parent(_parent)
        {
            parent->push();
        }

        impliciter_t(implicit_value_t *_parent, const T& t)
            : parent(_parent)
        {
            parent->push(t);
        }

        ~impliciter_t() {
            parent->pop();
        }
    private:
        implicit_value_t *parent;
    };

    bool has_value() {
        return scopes.front();
    }

    T get_value() {
        return *scopes.front();
    }

private:
    typedef std::deque<boost::optional<T> > scopes_t;
    scopes_t scopes;
};

#endif
