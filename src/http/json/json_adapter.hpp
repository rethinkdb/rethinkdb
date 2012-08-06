#ifndef HTTP_JSON_JSON_ADAPTER_HPP_
#define HTTP_JSON_JSON_ADAPTER_HPP_

#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include "containers/uuid.hpp"
#include "http/json.hpp"

/* A note about json adapter exceptions: When an operation throws an exception
 * there is no guaruntee that the target object has been left in tact.
 * Generally this is okay because we first apply changes and then join them in
 * to semilattice metadata. Generally once a particular object has thrown one
 * of these exceptions it should probably not be used anymore. */
struct json_adapter_exc_t : public std::exception {
    virtual const char *what() const throw () {
        return "Generic json adapter exception\n";
    }

    virtual ~json_adapter_exc_t() throw () { }
};

struct schema_mismatch_exc_t : public json_adapter_exc_t {
private:
    std::string desc;
public:
    explicit schema_mismatch_exc_t(const std::string &_desc)
        : desc(_desc)
    { }

    const char *what() const throw () {
        return desc.c_str();
    }

    ~schema_mismatch_exc_t() throw () { }
};

struct permission_denied_exc_t : public json_adapter_exc_t {
private:
    std::string desc;
public:
    explicit permission_denied_exc_t(const std::string &_desc)
        : desc(_desc)
    { }

    const char *what() const throw () {
        return desc.c_str();
    }

    ~permission_denied_exc_t() throw () { }
};

struct multiple_choices_exc_t : public json_adapter_exc_t {
    virtual const char *what() const throw () {
        return "Multiple choices exists for this json value (probably vector clock divergence).";
    }
};

//Type checking functions

bool is_null(cJSON *json);

//Functions to make accessing cJSON *objects easier

bool get_bool(cJSON *json);

std::string get_string(cJSON *json);

int get_int(cJSON *json);

double get_double(cJSON *json);

json_array_iterator_t get_array_it(cJSON *json);

json_object_iterator_t get_object_it(cJSON *json);

template <class ctx_t>
class subfield_change_functor_t {
public:
    virtual void on_change(const ctx_t &) = 0;
    virtual ~subfield_change_functor_t() { }
};

template <class ctx_t>
class noop_subfield_change_functor_t : public subfield_change_functor_t<ctx_t> {
public:
    void on_change(const ctx_t &) { }
};

template <class T, class ctx_t>
class standard_subfield_change_functor_t : public subfield_change_functor_t<ctx_t>{
private:
    T *target;
public:
    explicit standard_subfield_change_functor_t(T *);
    void on_change(const ctx_t &);
};

//TODO come up with a better name for this
template <class ctx_t>
class json_adapter_if_t {
public:
    typedef std::map<std::string, boost::shared_ptr<json_adapter_if_t> > json_adapter_map_t;

public:
    json_adapter_if_t() { }
    virtual ~json_adapter_if_t() { }

    json_adapter_map_t get_subfields(const ctx_t &);
    cJSON *render(const ctx_t &);
    void apply(cJSON *, const ctx_t &);
    void erase(const ctx_t &);
    void reset(const ctx_t &);

private:
    virtual json_adapter_map_t get_subfields_impl(const ctx_t &) = 0;
    virtual cJSON *render_impl(const ctx_t &) = 0;
    virtual void apply_impl(cJSON *, const ctx_t &) = 0;
    virtual void erase_impl(const ctx_t &) = 0;
    virtual void reset_impl(const ctx_t &) = 0;
    /* follows the creation paradigm, ie the caller is responsible for the
     * object this points to */
    virtual boost::shared_ptr<subfield_change_functor_t<ctx_t> >  get_change_callback() = 0;

    std::vector<boost::shared_ptr<subfield_change_functor_t<ctx_t> > > superfields;

    DISABLE_COPYING(json_adapter_if_t);
};

/* A json adapter is the most basic adapter, you can instantiate one with any
 * type that implements the json adapter concept as T */
template <class T, class ctx_t>
class json_adapter_t : public json_adapter_if_t<ctx_t> {
private:
    typedef typename json_adapter_if_t<ctx_t>::json_adapter_map_t json_adapter_map_t;
public:
    json_adapter_t(T *, const ctx_t &);

private:
    json_adapter_map_t get_subfields_impl(const ctx_t &);
    cJSON *render_impl(const ctx_t &);
    virtual void apply_impl(cJSON *, const ctx_t &);
    virtual void erase_impl(const ctx_t &);
    virtual void reset_impl(const ctx_t &);
    boost::shared_ptr<subfield_change_functor_t<ctx_t> > get_change_callback();

    T *target_;
    const ctx_t ctx_;

    DISABLE_COPYING(json_adapter_t);
};

/* A read only adapter is like a normal adapter but it throws an exception when
 * you try to do an apply call. */
template <class T, class ctx_t>
class json_read_only_adapter_t : public json_adapter_t<T, ctx_t> {
public:
    json_read_only_adapter_t(T *, const ctx_t &);

private:
    void apply_impl(cJSON *, const ctx_t &);
    void erase_impl(const ctx_t &);
    void reset_impl(const ctx_t &);

    DISABLE_COPYING(json_read_only_adapter_t);
};

/* A json temporary adapter is like a read only adapter but it stores a copy of
 * the what it's adapting inside it. This is convenient when we want to have
 * json data that's not actually reflected in our structures such as having the
 * id of every element in a map referenced in an id field */
template <class T, class ctx_t>
class json_temporary_adapter_t : public json_read_only_adapter_t<T, ctx_t> {
public:
    json_temporary_adapter_t(const T &value, const ctx_t &ctx);

private:
    T value_;

    DISABLE_COPYING(json_temporary_adapter_t);
};

/* A json_combiner_adapter_t is useful for glueing different adapters together.
 * */
template <class ctx_t>
class json_combiner_adapter_t : public json_adapter_if_t<ctx_t> {
private:
    typedef typename json_adapter_if_t<ctx_t>::json_adapter_map_t json_adapter_map_t;
public:
    explicit json_combiner_adapter_t(const ctx_t &ctx);
    void add_adapter(std::string key, boost::shared_ptr<json_adapter_if_t<ctx_t> > adapter);
private:
    cJSON *render_impl(const ctx_t &);
    void apply_impl(cJSON *, const ctx_t &);
    void erase_impl(const ctx_t &);
    void reset_impl(const ctx_t &);
    json_adapter_map_t get_subfields_impl(const ctx_t &);
    boost::shared_ptr<subfield_change_functor_t<ctx_t> > get_change_callback();

    json_adapter_map_t sub_adapters_;
    const ctx_t ctx_;

    DISABLE_COPYING(json_combiner_adapter_t);
};

/* This adapter is a little bit different from the other ones, it's meant to
 * target a map and allow a way to insert in to it, using serveside generated
 * keys. Because of this the render and apply functions have different schemas
 * in particular:
 *
 *  If target is of type map<K,V>
 *  schema(render) = schema(render_as_json(map<K,V>))
 *  schema(apply) = shchema(apply_json_to(V))
 *
 *  Rendering an inserter will give you only the entries in the map that were
 *  created using this inserter. Thus rendering with a newly constructed
 *  inserter gives you an empty map.
 */
template <class container_t, class ctx_t>
class json_map_inserter_t : public json_adapter_if_t<ctx_t> {
    typedef typename json_adapter_if_t<ctx_t>::json_adapter_map_t json_adapter_map_t;
    typedef boost::function<typename container_t::key_type()> gen_function_t;
    typedef typename container_t::mapped_type value_t;
    typedef std::set<typename container_t::key_type> keys_set_t;

public:
    json_map_inserter_t(container_t *, gen_function_t, const ctx_t &ctx, value_t _initial_value = value_t());

private:
    cJSON *render_impl(const ctx_t &);
    void apply_impl(cJSON *, const ctx_t &);
    void erase_impl(const ctx_t &);
    void reset_impl(const ctx_t &);
    json_adapter_map_t get_subfields_impl(const ctx_t &);
    boost::shared_ptr<subfield_change_functor_t<ctx_t> > get_change_callback();

    container_t *target;
    gen_function_t generator;
    value_t initial_value;
    keys_set_t added_keys;
    const ctx_t ctx;

    DISABLE_COPYING(json_map_inserter_t);
};

/* This combines the inserter json adapter with the standard adapter for a map,
 * thus creating an adapter for a map with which we can do normal modifications
 * and insertions */
template <class container_t, class ctx_t>
class json_adapter_with_inserter_t : public json_adapter_if_t<ctx_t> {
    typedef typename json_adapter_if_t<ctx_t>::json_adapter_map_t json_adapter_map_t;
    typedef boost::function<typename container_t::key_type()> gen_function_t;
    typedef typename container_t::mapped_type value_t;
public:
    json_adapter_with_inserter_t(container_t *, gen_function_t, const ctx_t &ctx, value_t _initial_value = value_t(), std::string _inserter_key = std::string("new"));
private:
    json_adapter_map_t get_subfields_impl(const ctx_t &);
    cJSON *render_impl(const ctx_t &);
    void apply_impl(cJSON *, const ctx_t &);
    void erase_impl(const ctx_t &);
    void reset_impl(const ctx_t &);
    void on_change(const ctx_t &);
    boost::shared_ptr<subfield_change_functor_t<ctx_t> > get_change_callback();
private:
    container_t *target;
    gen_function_t generator;
    value_t initial_value;
    std::string inserter_key;
    const ctx_t ctx;

    DISABLE_COPYING(json_adapter_with_inserter_t);
};

/* Erase is a fairly rare function for an adapter to allow so we implement a
 * generic version of it. */
template <class T, class ctx_t>
void erase_json(T *, const ctx_t &) {
#ifndef NDEBUG
    throw permission_denied_exc_t("Can't erase this object.. by default"
            "json_adapters disallow deletion. if you'd like to be able to please"
            "implement a working erase method for it.");
#else
    throw permission_denied_exc_t("Can't erase this object.");
#endif
}

/* Erase is a fairly rare function for an adapter to allow so we implement a
 * generic version of it. */
template <class T, class ctx_t>
void reset_json(T *, const ctx_t &) {
#ifndef NDEBUG
    throw permission_denied_exc_t("Can't reset this object.. by default"
            "json_adapters disallow deletion. if you'd like to be able to please"
            "implement a working reset method for it.");
#else
    throw permission_denied_exc_t("Can't reset this object.");
#endif
}


/* Here we have implementations of the json adapter concept for several
 * prominent types, these could in theory be relocated to a different file if
 * need be */

//JSON adapter for int
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(int *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(int *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, int *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(int *, const ctx_t &);

//JSON adapter for time_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(time_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(time_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, time_t *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(time_t *, const ctx_t &);

//JSON adapter for uint64_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(uint64_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(uint64_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, uint64_t *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(uint64_t *, const ctx_t &);

//JSON adapter for char
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(char *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(char *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, char *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(char *, const ctx_t &);

//JSON adapter for bool
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(bool *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(bool *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, bool *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(bool *, const ctx_t &);

//JSON adapter for uuid_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(uuid_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(const uuid_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, uuid_t *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(uuid_t *, const ctx_t &);

namespace boost {

//JSON adapter for boost::optional
template <class T, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(boost::optional<T> *, const ctx_t &);

template <class T, class ctx_t>
cJSON *render_as_json(boost::optional<T> *, const ctx_t &);

template <class T, class ctx_t>
void apply_json_to(cJSON *, boost::optional<T> *, const ctx_t &);

template <class T, class ctx_t>
void on_subfield_change(boost::optional<T> *, const ctx_t &);

//JSON adapter for boost::variant
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *, const ctx_t &);

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class ctx_t>
cJSON *render_as_json(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *, const ctx_t &);

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class ctx_t>
void apply_json_to(cJSON *, boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *, const ctx_t &);

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class ctx_t>
void on_subfield_change(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *, const ctx_t &);
} //namespace boost

namespace std {
//JSON adapter for std::string
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(std::string *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(std::string *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, std::string *, const ctx_t &);

template <class ctx_t>
void  on_subfield_change(std::string *, const ctx_t &);

//JSON adapter for std::map
template <class K, class V, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(std::map<K, V> *, const ctx_t &);

template <class K, class V, class ctx_t>
cJSON *render_as_json(std::map<K, V> *, const ctx_t &);

template <class K, class V, class ctx_t>
void apply_json_to(cJSON *, std::map<K, V> *, const ctx_t &);

template <class K, class V, class ctx_t>
void on_subfield_change(std::map<K, V> *, const ctx_t &);

//JSON adapter for std::set
template <class V, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(std::set<V> *, const ctx_t &);

template <class V, class ctx_t>
cJSON *render_as_json(std::set<V> *, const ctx_t &);

template <class V, class ctx_t>
void apply_json_to(cJSON *, std::set<V> *, const ctx_t &);

template <class V, class ctx_t>
void on_subfield_change(std::set<V> *, const ctx_t &);

//JSON adapter for std::pair
template <class F, class S, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(std::pair<F, S> *, const ctx_t &);

template <class F, class S, class ctx_t>
cJSON *render_as_json(std::pair<F, S> *, const ctx_t &);

template <class F, class S, class ctx_t>
void apply_json_to(cJSON *, std::pair<F, S> *, const ctx_t &);

template <class F, class S, class ctx_t>
void on_subfield_change(std::pair<F, S> *, const ctx_t &);

//JSON adapter for std::vector
template <class V, class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(std::vector<V> *, const ctx_t &);

template <class V, class ctx_t>
cJSON *render_as_json(std::vector<V> *, const ctx_t &);

template <class V, class ctx_t>
void apply_json_to(cJSON *, std::vector<V> *, const ctx_t &);

template <class V, class ctx_t>
void on_subfield_change(std::vector<V> *, const ctx_t &);
} //namespace std

//some convenience functions
template <class T, class ctx_t>
cJSON *render_as_directory(T *, const ctx_t &);

template <class T, class ctx_t>
void apply_as_directory(cJSON *change, T *, const ctx_t &);

template<class T, class cxt_t>
std::string render_as_json_string(const T &t, const cxt_t cxt) {
    T copy = t;
    scoped_cJSON_t json(render_as_json(&copy, cxt));
    return json.PrintUnformatted();
}

template<class T>
std::string render_as_json_string(const T &t) {
    return render_as_json_string(t, 0);
}

#include "http/json/json_adapter.tcc"

#endif /* HTTP_JSON_JSON_ADAPTER_HPP_ */
