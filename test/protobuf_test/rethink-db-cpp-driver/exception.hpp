#ifndef RETHINK_DB_DRIVER_EXCEPTION
#define RETHINK_DB_DRIVER_EXCEPTION

#include <stdexcept>
#include <string>

using namespace std;

namespace com {
	namespace rethinkdb {
		namespace driver {

			class connection_exception : public runtime_error {
			public:
				connection_exception() : runtime_error(""){}
				connection_exception(const string& msg) : runtime_error(("RethinkDB connection exception. " + msg).c_str()){}
			};

			class runtime_error_exception : public runtime_error {
			public:
				runtime_error_exception() : runtime_error(""){}
				runtime_error_exception(const string& msg) : runtime_error(("RethinkDB runtime exception. " + msg).c_str()){}
			};

			class compile_error_exception : public runtime_error {
			public:
				compile_error_exception() : runtime_error(""){}
				compile_error_exception(const string& msg) : runtime_error(("RethinkDB compile error exception. " + msg).c_str()){}
			};

			class client_error_exception : public runtime_error {
			public:
				client_error_exception() : runtime_error(""){}
				client_error_exception(const string& msg) : runtime_error(("RethinkDB client error exception. " + msg).c_str()){}
			};
		}
	}
}
#endif