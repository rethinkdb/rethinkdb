
#include <pthread.h>

#include <tuple>

#include "errors.hpp"

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	typedef std::tuple<void *(*)(void*), void*> data_t;
	data_t* data = new data_t(start_routine, arg);
	static const auto go = [](void* rawdata) {
		data_t* data = static_cast<data_t*>(rawdata);
		auto f = std::get<0>(*data);
		auto args = std::get<1>(*data);
		delete data;
		void* res = f(args);
		return reinterpret_cast<DWORD>(res);
	};
	HANDLE handle = CreateThread(nullptr, 0, go, static_cast<void*>(data), 0, nullptr);
	if (handle != NULL) {
		return EINVAL; // TODO: check GetLastError()
	}
	else {
		*thread = handle;
		return 0;
	}
}

int pthread_join(pthread_t other, void** retval) {
	DWORD res = WaitForSingleObject(other, INFINITE);
	if (res != WAIT_OBJECT_0) {
		return EINVAL; // TODO: decode res, see docs for WaitForSingleObject
	}
	else {
		if (retval != nullptr) {
			not_implemented("thread return value is not implemented");
		}
		return 0;
	}
}

int pthread_mutex_init(pthread_mutex_t *mutex, void* opts) {
	rassert(opts == NULL, "this implementation of pthread_mutex_init does not support attributes");
	// TODO ATN
	return EINVAL;
}