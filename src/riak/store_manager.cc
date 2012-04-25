#include "riak/store_manager.hpp"

#include "serializer/config.hpp"

// OH LOOK A CC FILE!  WHO'D 'A THUNK IT?!


// This visitor must be extended if new types of store configs are added
class store_loader_t : public boost::static_visitor<store_object_t> {
public:
    store_object_t operator()(standard_serializer_t::config_t &config) const {
        struct : public promise_t<bool>,
                 public log_serializer_t::check_callback_t
        {
            void on_serializer_check(bool is_existing) { pulse(is_existing); }
        } existing_cb;
        log_serializer_t::check_existing(config.private_dynamic_config.db_filename.c_str(), &existing_cb);

        if (!existing_cb.wait()) {
            log_serializer_t::create(config.dynamic_config, config.private_dynamic_config, config.static_config);
        }
        return store_object_t(boost::shared_ptr<standard_serializer_t>(
                new standard_serializer_t(config.dynamic_config, config.private_dynamic_config)));
    }

    // Add implementations for other store_config types here...
    store_object_t operator()(invalid_variant_t) const {
        crash("Tried to create an invalid store\n");
        return store_object_t();
    }
};

// TODO: Is this seriously a globally namespaced type named store_t?  Seriously??
void store_t::load_store() {
    // First unload the current store, if any exists
    // (this is useful in case the current and new store are using the same
    //  underlying files, for example if you just changed some configuration
    //  option in the store_config and want to re-create the store to apply
    //  those changes)
    store = boost::shared_ptr<invalid_variant_t>();
    // Now load the new store
    store = boost::apply_visitor(store_loader_t(), store_config);
}
