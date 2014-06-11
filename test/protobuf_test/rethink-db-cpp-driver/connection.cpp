#include <boost/format.hpp>
#include "exception.hpp"
#include "connection.hpp"

namespace com {
	namespace rethinkdb {
		namespace driver {

			connection::connection(const string& host, const string& port, const string& database, const string& auth_key) : resolver_(io_service), socket_(io_service) {
				this->host = host;
				this->port = port;
				this->database = database;
				this->auth_key = auth_key;
				this->connection_established = false;
			}

			bool connection::is_connected() {
				return (socket_.is_open() && this->connection_established);
			}

			bool connection::reconnect() {
				return this->connect();
			}

			bool connection::connect() {

				if (this->is_connected()) return true;

				string response;			

				try {
					// resolve the host
					ip::tcp::resolver::query query(this->host, this->port);
					ip::tcp::resolver::iterator iterator = this->resolver_.resolve(query);
					boost::asio::connect(this->socket_, iterator);

					// prepare stream for writing data
					ostream request_stream(&(this->request_));

					// write magic version number
					request_stream.write((char*)&(VersionDummy::V0_2), sizeof (VersionDummy::V0_2));

					// write auth_key length
					u_int auth_key_length = this->auth_key.length();
					request_stream.write((char*)&auth_key_length, sizeof (u_int));

					// write auth_key
					request_stream.write(this->auth_key.c_str(), auth_key.length());

					// send the request
					write(this->socket_, this->request_);

					// read response until a null character 
					read_until(this->socket_, this->response_, 0);

					// prepare to read a response
					istream response_stream(&(this->response_));

					// read one line of response
					getline(response_stream, response);
				}
				catch (exception& e)
				{
					// exception from boost has been caught
					throw connection_exception(string(e.what()));
				}

				// if it starts with "SUCCESS"
				if (response.substr(0, 7) == "SUCCESS") {
					this->connection_established = true;
					return true;
				}
				else {
					this->connection_established = false;
					throw connection_exception(response);
				}

			}

			void connection::close() {
				this->socket_.close();
				this->connection_established = false;
			}

			/* -------------------------------------------------------------------- */

			void connection::use(const string& db_name) {
				this->database = db_name;
			}

			/* -------------------------------------------------------------------- */

			shared_ptr<Response> connection::read_response() {
				u_int response_length;
				char* reply;
				size_t bytes_read;
				shared_ptr<Response> response(new Response());

				try {
					try {
						// read length of the response
						read(this->socket_, buffer(&response_length, sizeof(u_int)));

						// read content
						reply = new char[response_length];
						bytes_read = read(this->socket_, buffer(reply, response_length));
					}
					catch (exception&) {
						throw connection_exception("Unable to read from the socket.");
					}

					if (bytes_read != response_length) throw connection_exception(boost::str(boost::format("%1% bytes read, when %2% bytes promised.") % bytes_read % response_length));

					try {
						response->ParseFromArray(reply, response_length);
					}
					catch (exception&) {
						throw connection_exception("Unable to parse the protobuf Response object.");
					}

					delete[] reply;
				}
				catch (exception& e) {
					throw connection_exception(boost::str(boost::format("Read response: %1%") % e.what()));
				}

				return response;
			}

			void connection::write_query(const Query& query) {
				// prepare output stream
				ostream request_stream(&request_);

				// serialize query
				string blob = query.SerializeAsString();

				// write blob's length in little endian 32 bit unsigned integer
				u_int blob_length = blob.length();
				request_stream.write((char*)&blob_length, sizeof (u_int));

				// write protobuf blob
				request_stream.write(blob.c_str(), blob.length());

				try {
					// send the content of the output stream over the wire
					write(this->socket_, this->request_);
				}
				catch (exception& e)
				{
					throw connection_exception(boost::str(boost::format("Write query: exception: %1%") % e.what()));
				}
			}

			/* -------------------------------------------------------------------- */

			shared_ptr<RQL> connection::r() {
				shared_ptr<RQL> ptr(new RQL());
				ptr->conn = shared_from_this();
				return ptr;
			}
		}
	}
}