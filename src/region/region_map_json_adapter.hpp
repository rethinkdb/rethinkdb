#ifndef REGION_REGION_MAP_JSON_ADAPTER_HPP_
#define REGION_REGION_MAP_JSON_ADAPTER_HPP_

#include <string>

#include "region/region_json_adapter.hpp"
#include "region/region_map.hpp"

/* A ctx-free special adapter for a region_map's value */
template <class value_t>
class json_region_adapter_t : public json_adapter_if_t {
private:
public:
    json_region_adapter_t(region_map_t<value_t> *_parent, region_t _target_region)
        : parent(_parent), target_region(_target_region) { }
private:
    json_adapter_if_t::json_adapter_map_t get_subfields_impl() {
            return json_adapter_if_t::json_adapter_map_t();
    }

    cJSON *render_impl() {
        region_map_t<value_t> target_map = parent->mask(target_region);
        return render_as_json(&target_map);
    }

    void apply_impl(cJSON *change) {
        value_t val;
        apply_json_to(change, &val);

        parent->set(target_region, val);
    }

    void erase_impl() {
        throw permission_denied_exc_t("Can't erase from a region map.\n");
    }

    void reset_impl() {
        throw permission_denied_exc_t("Can't reset from a region map.\n");
    }

    /* follows the creation paradigm, ie the caller is responsible for the
     * object this points to */
    boost::shared_ptr<subfield_change_functor_t>  get_change_callback() {
        return boost::shared_ptr<subfield_change_functor_t>(new noop_subfield_change_functor_t());
    }

    region_map_t<value_t> *parent;
    region_t target_region;

    DISABLE_COPYING(json_region_adapter_t);
};

//json adapter concept for region map
template <class value_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(region_map_t<value_t> *target) {
    json_adapter_if_t::json_adapter_map_t res;
    for (typename region_map_t<value_t>::iterator it = target->begin();
         it != target->end();
         ++it) {
        scoped_cJSON_t key(render_as_json(&it->first));
        guarantee(key.get()->type == cJSON_String);
        res[get_string(key.get())] = boost::shared_ptr<json_adapter_if_t>(new json_region_adapter_t<value_t>(target, it->first));
    }

    return res;
}

template <class value_t>
cJSON *render_as_json(region_map_t<value_t> *target) {
    cJSON *res = cJSON_CreateObject();
    for (typename region_map_t<value_t>::iterator it = target->begin();
         it != target->end();
         ++it) {
        std::string key(render_region_as_string(&it->first));
        cJSON_AddItemToObject(res, key.c_str(), render_as_json(&it->second));
    }

    return res;
}

template <class value_t>
void apply_json_to(cJSON *change, region_map_t<value_t> *target) {
    json_object_iterator_t it = get_object_it(change);
    cJSON *hd;

    while ((hd = it.next())) {
       region_t key;
       value_t val;

       scoped_cJSON_t key_desc(cJSON_CreateString(hd->string));
       apply_json_to(key_desc.get(), &key);
       apply_json_to(hd, &val);

       target->set(key, val);
    }
}


#endif  // REGION_REGION_MAP_JSON_ADAPTER_HPP_
