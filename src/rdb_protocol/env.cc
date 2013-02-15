#include "rdb_protocol/env.hpp"

namespace ql {

bool env_t::add_optarg(const std::string &key, term_t *val) {
    if (optargs.count(key)) return true;
    optargs[key] = val;
    return false;
}
val_t *env_t::get_optarg(const std::string &key){
    if (!optargs.count(key)) return 0;
    return optargs[key]->eval(true);
}


int env_t::gensym() {
    r_sanity_check(next_gensym_val > -100000);
    return next_gensym_val--;
}

void env_t::push_implicit(const datum_t **val) {
    implicit_var.push(val);
}
const datum_t **env_t::top_implicit() {
    rcheck(!implicit_var.empty(), "No implicit variable in scope.");
    rcheck(implicit_var.size() == 1,
           "Cannot use implicit variable in nested queries; name your variables.");
    return implicit_var.top();
}
void env_t::pop_implicit() {
    implicit_var.pop();
}

size_t env_t::num_checkpoints() {
    return bags.size()-1;
}
bool env_t::some_bag_has(const ptr_baggable_t *p) {
    for (size_t i = 0; i < bags.size(); ++i) if (bags[i]->has(p)) return true;
    return false;
}
void env_t::checkpoint() {
    bags.push_back(new ptr_bag_t());
}

void env_t::merge_checkpoint() {
    r_sanity_check(bags.size() >= 2);
    bags[bags.size()-2]->add(bags[bags.size()-1]);
    bags.pop_back();
}
void env_t::discard_checkpoint() {
    r_sanity_check(bags.size() >= 2);
    delete bags[bags.size()-1];
    bags.pop_back();
}

bool env_t::gc_callback_trampoline(const datum_t *el, env_t *env) {
    return env->gc_callback(el);
}
bool env_t::gc_callback(const datum_t *el) {
    if (old_bag->has(el)) {
        old_bag->yield_to(new_bag, el);
        return true;
    }
    r_sanity_check(some_bag_has(el));
    return false;
}
void env_t::gc(const datum_t *root) {
    old_bag = get_bag();
    scoped_ptr_t<ptr_bag_t> _new_bag(new ptr_bag_t);
    new_bag = _new_bag.get();
    // debugf("GC {\n");
    // debugf("  old_bag: %s\n", old_bag->print_debug().c_str());
    // debugf("  new_bag: %s\n", new_bag->print_debug().c_str());
    root->iter(gc_callback_trampoline, this);
    // debugf(" --- \n");
    // debugf("  old_bag: %s\n", old_bag->print_debug().c_str());
    // debugf("  new_bag: %s\n", new_bag->print_debug().c_str());
    // debugf("}\n");
    *get_bag_ptr() = _new_bag.release();
    delete old_bag;
}

ptr_bag_t *env_t::get_bag() { return *get_bag_ptr(); }
ptr_bag_t **env_t::get_bag_ptr() {
    r_sanity_check(bags.size() > 0);
    return &bags[bags.size()-1];
}

void env_t::push_var(int var, const datum_t **val) {
    // debugf("%p VAR push %d -> %p\n", this, var, val);
    vars[var].push(val);
}
const datum_t **env_t::top_var(int var) {
    // if (vars[var].empty()) debugf("%p VAR top %d -> FAILED\n", this, var);
    rcheck(!vars[var].empty(),
           strprintf("Unrecognized variabled %d", var));
    // debugf("%p VAR top %d -> %p\n", this, var, vars[var].top());
    return vars[var].top();
}
void env_t::pop_var(int var) {
    // debugf("%p VAR pop %d (%p)\n", this, var, vars[var].top());
    vars[var].pop();
}
void env_t::dump_scope(std::map<int, const datum_t **> *out) {
    for (std::map<int, std::stack<const datum_t **> >::iterator
             it = vars.begin(); it != vars.end(); ++it) {
        if (it->second.size() == 0) continue;
        r_sanity_check(it->second.top());
        (*out)[it->first] = it->second.top();
    }
}
void env_t::push_scope(std::map<int, Datum> *in) {
    scope_stack.push(std::vector<std::pair<int, const datum_t *> >());

    for (std::map<int, Datum>::iterator it = in->begin(); it != in->end(); ++it) {
        const datum_t *d = add_ptr(new datum_t(&it->second, this));
        scope_stack.top().push_back(std::make_pair(it->first, d));
    }

    for (size_t i = 0; i < scope_stack.top().size(); ++i) {
        // debugf("%p -> %p\n",
        //        &scope_stack.top()[i].second,
        //        scope_stack.top()[i].second);
        push_var(scope_stack.top()[i].first, &scope_stack.top()[i].second);
    }
}
void env_t::pop_scope() {
    r_sanity_check(scope_stack.size() > 0);
    for (size_t i = 0; i < scope_stack.top().size(); ++i) {
        pop_var(scope_stack.top()[i].first);
    }
    // DO NOT pop the vector off the scope stack.  You might invalidate a
    // pointer too early.
}

env_checkpoint_t::env_checkpoint_t(env_t *_env, void (env_t::*_f)())
    : env(_env) , f(_f) {
    env->checkpoint();
}
env_checkpoint_t::~env_checkpoint_t() { (env->*f)(); }
void env_checkpoint_t::reset(void (env_t::*_f)()) {
    f = _f;
}
void env_checkpoint_t::gc(const datum_t *root) {
    env->gc(root);
}

// We GC more frequently (~ every 16 data) in debug mode to help with testing.
int env_gc_checkpoint_t::default_gen1_cutoff = (8 * 1024 * 1024)
    DEBUG_ONLY(* 0 + (sizeof(datum_t) * ptr_bag_t::mem_estimate_mul * 16));

env_gc_checkpoint_t::env_gc_checkpoint_t(env_t *_env, size_t _gen1, size_t _gen2)
    : finalized(false), env(_env), gen1(_gen1), gen2(_gen2) {
    r_sanity_check(env);
    if (!gen1) gen1 = default_gen1_cutoff;
    if (!gen2) gen2 = 8 * gen1;
    env->checkpoint(); // gen2
    env->checkpoint(); // gen1
}
env_gc_checkpoint_t::~env_gc_checkpoint_t() {
    // We might not be finalized if e.g. an exception was thrown.
    if (!finalized) {
        env->merge_checkpoint();
        env->merge_checkpoint();
    }
}
const datum_t *env_gc_checkpoint_t::maybe_gc(const datum_t *root) {
    if (env->get_bag()->mem_estimate() > gen1) {
        env->gc(root);
        env->merge_checkpoint();
        if (env->get_bag()->mem_estimate() > gen2) {
            env->gc(root);
            if (env->get_bag()->mem_estimate() > (gen2 * 2 / 3)) gen2 *= 4;
        }
        env->checkpoint();
    }
    return root;
}
const datum_t *env_gc_checkpoint_t::finalize(const datum_t *root) {
    finalized = true;
    if (root) env->gc(root);
    env->merge_checkpoint();
    if (root) env->gc(root);
    env->merge_checkpoint();
    return root;
}

} // namespace ql
