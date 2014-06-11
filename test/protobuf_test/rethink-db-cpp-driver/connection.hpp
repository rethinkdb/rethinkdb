#ifndef RETHINK_DB_DRIVER_CONNECTION
#define RETHINK_DB_DRIVER_CONNECTION

#include "proto/rdb_protocol/ql2.pb.h"
#include <boost/asio.hpp>
#include <string>
#include "rql.hpp"

using namespace std;
using namespace boost::asio;
using namespace com::rethinkdb;
using namespace com::rethinkdb::driver;

namespace com {
	namespace rethinkdb {
		namespace driver {
			
			class connection : public enable_shared_from_this<connection> {

			public:

				connection(const string& host, const string& port, const string& database, const string& auth_key);
				
				bool is_connected();
				bool reconnect();
				bool connect();
				void close();
				
				/* -------------------------------------------------------------------- */

				void use(const string& db_name);

				/* -------------------------------------------------------------------- */

				shared_ptr<Response> read_response();
				void write_query(const Query& query);

				/* -------------------------------------------------------------------- */

				shared_ptr<RQL> r();

				string database;

			private:

				string host;
				string port;
				string auth_key;
				bool connection_established;

				boost::asio::io_service io_service;
				ip::tcp::resolver resolver_;
				ip::tcp::socket socket_;
				boost::asio::streambuf request_;
				boost::asio::streambuf response_;
			};

		}
	}
}
#endif
