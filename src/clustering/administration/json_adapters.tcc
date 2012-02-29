#ifndef __CLUSTERING_ADMINISTRATION_JSON_ADAPTERS_TCC__
#define __CLUSTERING_ADMINISTRATION_JSON_ADAPTERS_TCC__

#include "http/json.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "stl_utils.hpp"

//json adapter concept for vclock_t
template <class T, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(vclock_t<T> *target, const ctx_t &ctx) {
    try {
        return get_json_subfields(&target->get_mutable(), ctx);
    } catch (typename vclock_t<T>::in_conflict_exc_t e) { 
        return typename json_adapter_if_t<ctx_t>::json_adapter_map_t();
    }
}

template <class T, class ctx_t>
cJSON *render_as_json(vclock_t<T> *target, const ctx_t &ctx) {
    try {
        T t = target->get();
        return render_as_json(&t, ctx);
    } catch (typename vclock_t<T>::in_conflict_exc_t e) {
        return cJSON_CreateString("Value in conflict\n");
    }
}

template <class T, class ctx_t>
cJSON *render_all_values(vclock_t<T> *target, const ctx_t &ctx) {
    cJSON *res = cJSON_CreateArray();
    for (typename vclock_t<T>::value_map_t::iterator it  = target->values.begin();
                                                             it != target->values.end();
                                                             it++) {
        cJSON *version_value_pair = cJSON_CreateArray();

        //This is really bad, fix this as soon as we can render using non const references
        vclock_details::version_map_t tmp = it->first;
        cJSON_AddItemToArray(res, render_as_json(&tmp, ctx));
        cJSON_AddItemToArray(res, render_as_json(&it->second, ctx));

        cJSON_AddItemToArray(res, version_value_pair);
    }

    return res;
}

template <class T, class ctx_t>
void apply_json_to(cJSON *change, vclock_t<T> *target, const ctx_t &ctx) {
    try {
        apply_json_to(change, &target->get_mutable(), ctx);
        target->upgrade_version(ctx.us);
    } catch (typename vclock_t<T>::in_conflict_exc_t e) {
        throw multiple_choices_exc_t();
    }
}

template <class T, class ctx_t>
void on_subfield_change(vclock_t<T> *target, const ctx_t &ctx) {
    target->upgrade_version(ctx.us);
}


template <class T, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t json_vclock_resolver_t<T, ctx_t>::get_subfields_impl(const ctx_t &) {
    return typename json_adapter_if_t<ctx_t>::json_adapter_map_t();
}

template <class T, class ctx_t>
cJSON *json_vclock_resolver_t<T, ctx_t>::render_impl(const ctx_t &ctx) {
    return render_all_values(target, ctx);
}

template <class T, class ctx_t>
void json_vclock_resolver_t<T, ctx_t>::apply_impl(cJSON *change, const ctx_t &ctx) {
    T new_value;
    apply_json_to(change, &new_value, ctx);

    *target = target->make_resolving_version(new_value, ctx.us);
}

template <class T, class ctx_t>
boost::shared_ptr<subfield_change_functor_t<ctx_t> > json_vclock_resolver_t<T, ctx_t>::get_change_callback() {
    return boost::shared_ptr<subfield_change_functor_t<ctx_t> >();
}

template <class T, class ctx_t>
json_vclock_resolver_t<T, ctx_t>::json_vclock_resolver_t(vclock_t<T> *_target) 
    : target(_target)
{ }

template <class T, class ctx_t>
json_vclock_adapter_t<T,ctx_t>::json_vclock_adapter_t(vclock_t<T> *_target)
    : target(_target)
{ }

template <class T, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t json_vclock_adapter_t<T,ctx_t>::get_subfields_impl(const ctx_t &ctx) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res = get_json_subfields(target, ctx);
    rassert(!std_contains("resolve", res), "Programmer error: do not put anything with a \"resolve\" subfield in a vector clock.\n");
    res["resolve"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_vclock_resolver_t<T, ctx_t>(target));

    return res;
}

template <class T, class ctx_t>
cJSON *json_vclock_adapter_t<T,ctx_t>::render_impl(const ctx_t &ctx) {
    return render_as_json(target, ctx);
}

template <class T, class ctx_t>
void json_vclock_adapter_t<T,ctx_t>::apply_impl(cJSON *change, const ctx_t &ctx) {
    apply_json_to(change, target, ctx);
}

template <class T, class ctx_t>
boost::shared_ptr<subfield_change_functor_t<ctx_t> >  json_vclock_adapter_t<T,ctx_t>::get_change_callback() {
    return boost::shared_ptr<subfield_change_functor_t<ctx_t> >(new standard_subfield_change_functor_t<vclock_t<T>, ctx_t>(target));
}

//json adapter concept for deletable_t
template <class T, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(deletable_t<T> *target, const ctx_t &ctx) {
    return get_json_subfields(&target->get_mutable(), ctx);
}

template <class T, class ctx_t>
cJSON *render_as_json(deletable_t<T> *target, const ctx_t &ctx) {
    return render_as_json(&target->get_mutable(), ctx);
}

template <class T, class ctx_t>
void apply_json_to(cJSON *change, deletable_t<T> *target, const ctx_t &ctx) {
    apply_json_to(change, &target->get_mutable(), ctx);
}

template <class T, class ctx_t>
void on_subfield_change(deletable_t<T> *, const ctx_t &) { }

//json adapter concept for peer_id_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(peer_id_t *, const ctx_t &) {
    return typename json_adapter_if_t<ctx_t>::json_adapter_map_t();
}

template <class ctx_t>
cJSON *render_as_json(peer_id_t *target, const ctx_t &ctx) {
    boost::uuids::uuid uuid = target->get_uuid();
    return render_as_json(&uuid, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, peer_id_t *target, const ctx_t &ctx) {
    boost::uuids::uuid uuid;
    apply_json_to(change, &uuid, ctx);
    *target = peer_id_t(uuid);
}

template <class ctx_t>
void on_subfield_change(peer_id_t *, const ctx_t &) { }

#endif
