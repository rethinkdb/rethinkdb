#ifndef RDB_PROTOCOL_NAMESPACE_INTERFACE_ACCESS_HPP_
#define RDB_PROTOCOL_NAMESPACE_INTERFACE_ACCESS_HPP_

#include "protocol_api.hpp"
#include "threading.hpp"

/* `namespace_interface_access_t` is like a smart pointer to a `namespace_interface_t`.
This is the format in which `real_table_t` expects to receive its
`namespace_interface_t *`. This allows the thing that is constructing the `real_table_t`
to control the lifetime of the `namespace_interface_t`, but also allows the
`real_table_t` to block it from being destroyed while in use. */
class namespace_interface_access_t {
public:
	class ref_tracker_t {
	public:
		virtual void add_ref() = 0;
		virtual void release() = 0;
	protected:
		virtual ~ref_tracker_t() { }
	};
	namespace_interface_access_t();
	namespace_interface_access_t(namespace_interface_t *, ref_tracker_t *, threadnum_t);
	namespace_interface_access_t(const namespace_interface_access_t &access);
	namespace_interface_access_t &operator=(const namespace_interface_access_t &access);
	~namespace_interface_access_t();

	namespace_interface_t *get();

private:
	namespace_interface_t *nif;
	ref_tracker_t *ref_tracker;
	threadnum_t thread;
};

#endif // RDB_PROTOCOL_NAMESPACE_INTERFACE_ACCESS_HPP_