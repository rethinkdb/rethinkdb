#include "exception.hpp"
#include "connection.hpp"
#include "rql.hpp"

namespace com {
	namespace rethinkdb {
		namespace driver {
			
			vector<shared_ptr<Response>> RQL::run() {
				conn->connect();

				// TODO - optargs?

				vector<shared_ptr<Response>> responses = vector<shared_ptr<Response>>();
				shared_ptr<Response> response;

				Query query;
				query.set_type(Query::QueryType::Query_QueryType_START);				
				// generate random token
				query.set_token(rand());

				*(query.mutable_query()) = term;
				
				// debugging
				//query.PrintDebugString();

				// write query
				conn->write_query(query);

				// read response
				response = conn->read_response();
				this->check_response(response);

				// if this point is reached, response is fine

				responses.push_back(response);

				query.set_type(Query::QueryType::Query_QueryType_CONTINUE);
				query.clear_query();

				while (response->type() == Response::ResponseType::Response_ResponseType_SUCCESS_PARTIAL) {
					// write query
					conn->write_query(query);

					// read response
					response = conn->read_response();
					this->check_response(response);

					responses.push_back(response);
				}

				return responses;
			}

			/* -------------------------------------------------------------------- */

			void RQL::check_response(shared_ptr<Response> response) {
				switch (response->type()) {
				case Response::ResponseType::Response_ResponseType_RUNTIME_ERROR:
					throw runtime_error_exception("\n\nResponse received:\n" + response->DebugString());
					break;
				case Response::ResponseType::Response_ResponseType_CLIENT_ERROR:
					throw client_error_exception("\n\nResponse received:\n" + response->DebugString());
					break;
				case Response::ResponseType::Response_ResponseType_COMPILE_ERROR:
					throw compile_error_exception("\n\nResponse received:\n" + response->DebugString());
					break;
				default:
					break;
				}
			}

			/* -------------------------------------------------------------------- */

			shared_ptr<RQL_Database> RQL::db(const RQL_String& db_name) {
				shared_ptr<RQL_Database> object(new RQL_Database());
				object->term.set_type(Term::TermType::Term_TermType_DB);
				*(object->term.add_args()) = db_name.term;
				object->conn = this->conn;
				return object;
			};

			shared_ptr<RQL_Database> RQL::db(const string& db_name) {
				return db(RQL_String(db_name));
			};

			shared_ptr<RQL_Array> RQL::db_list() {
				shared_ptr<RQL_Array> array(new RQL_Array());
				array->term.set_type(Term::TermType::Term_TermType_DB_LIST);
				array->conn = this->conn;
				return array;
			}

			shared_ptr<RQL_Object> RQL::db_create(const RQL_String& db_name) {
				shared_ptr<RQL_Object> object(new RQL_Object());
				object->term.set_type(Term::TermType::Term_TermType_DB_CREATE);
				*(object->term.add_args()) = db_name.term;
				object->conn = this->conn;
				return object;
			}

			shared_ptr<RQL_Object> RQL::db_create(const string& db_name) {
				return db_create(RQL_String(RQL_String(db_name)));
			}

			shared_ptr<RQL_Object> RQL::db_drop(const RQL_String& db_name) {
				shared_ptr<RQL_Object> object(new RQL_Object());
				object->term.set_type(Term::TermType::Term_TermType_DB_DROP);
				*(object->term.add_args()) = db_name.term;
				object->conn = this->conn;
				return object;
			}

			shared_ptr<RQL_Object> RQL::db_drop(const string& db_name) {
				return db_drop(RQL_String(RQL_String(db_name)));
			}

			/* -------------------------------------------------------------------- */

			shared_ptr<RQL_Table> RQL::table(const RQL_String& table_name) {
				return db(this->conn->database)->table(table_name);
			}

			shared_ptr<RQL_Table> RQL::table(const string& table_name) {
				return db(this->conn->database)->table(RQL_String(table_name));
			}

			shared_ptr<RQL_Object> RQL::table_create(const RQL_String& table_name) {
				return db(this->conn->database)->table_create(table_name);
			}

			shared_ptr<RQL_Object> RQL::table_create(const string& table_name) {
				return db(this->conn->database)->table_create(RQL_String(table_name));
			}

			shared_ptr<RQL_Array> RQL::table_drop(const RQL_String& table_name) {
				return db(this->conn->database)->table_drop(table_name);
			}

			shared_ptr<RQL_Array> RQL::table_drop(const string& table_name) {
				return db(this->conn->database)->table_drop(RQL_String(table_name));
			}

			shared_ptr<RQL_Array> RQL::table_list() {
				return db(this->conn->database)->table_list();
			}
		}
	}
}