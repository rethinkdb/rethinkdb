#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include "rethink_db.hpp"

#define default_host "10.211.55.2"
#define default_port "28015"
#define default_db "test"
#define default_auth_key ""

using namespace std;
using namespace com::rethinkdb;
using namespace com::rethinkdb::driver;

string ask(const string& description) {
	std::cout << "> " << description << ": ";
	string input = "";
	getline(cin, input);
	std::cout << endl;
	boost::algorithm::trim(input);
	return input;
}

int main(int argc, char* argv) {

	string host = ask("host (leave empty for '" default_host "')");
	if (host.empty()) host = default_host;
	string port = ask("port (leave empty for '" default_port "')");
	if (port.empty()) port = default_port;
	string db = ask("default database (leave empty for '" default_db "')");
	if (db.empty()) db = default_db;
	string auth_key = ask("authorization key (leave empty for '" default_auth_key "')");
	if (auth_key.empty()) auth_key = default_auth_key;

	shared_ptr <connection> conn(new connection(host, port, db, auth_key));
	vector<shared_ptr<Response>> responses = vector<shared_ptr<Response>>();
	
	string action;

	while (!cin.eof()) {
		try {
			responses = vector<shared_ptr<Response>>();
			std::cout << endl;
			std::cout << "========================================================================" << endl;
			std::cout << "    ACCESSING REQL: use" << endl;
			std::cout << "    MANIPULATING DATABASES: db_create db_drop db_list use" << endl;
			std::cout << "    MANIPULATING TABLES: table_create table_drop table_list" << endl;
			std::cout << "    WRITING DATA: insert remove update" << endl;
			std::cout << "    SELECTING DATA: between filter get table" << endl;
			std::cout << "    TRANSFORMATIONS: limit order_by skip" << endl;
			std::cout << "    exit" << endl;
			std::cout << "========================================================================" << endl << endl;
			std::cout << endl;

			action = ask("action");

			if (action.length() == 0) {
				std::cout << "Invalid action." << endl;
			}
			else if (action == "exit") {
				std::cout << "Exiting..." << endl << endl;
				break;
			}
			else if (action == "use") {
				conn->use(ask("db_name"));
				std::cout << endl << "Default database has been changed." << endl;
			}
			else if (action == "db_create") {
				responses = conn->r()->db_create(ask("db_name"))->run();
			}
			else if (action == "db_drop") {
				responses = conn->r()->db_drop(ask("db_name"))->run();
			}
			else if (action == "db_list") {
				responses = conn->r()->db_list()->run();
			}
			else if (action == "table") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				if (db_name == "") {
					responses = conn->r()->table(ask("table_name"))->run();
				}
				else {
					responses = conn->r()->db(db_name)->table(ask("table_name"))->run();
				}
			}
			else if (action == "table_create") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				if (db_name == "") {
					responses = conn->r()->table_create(ask("table_name"))->run();
				}
				else {
					responses = conn->r()->db(db_name)->table_create(ask("table_name"))->run();
				}
			}
			else if (action == "table_drop") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				if (db_name == "") {
					responses = conn->r()->table_drop(ask("table_name"))->run();
				}
				else {
					responses = conn->r()->db(db_name)->table_drop(ask("table_name"))->run();
				}
			}
			else if (action == "table_list") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				if (db_name == "") {
					responses = conn->r()->table_list()->run();
				}
				else {
					responses = conn->r()->db(db_name)->table_list()->run();
				}
			}
			else if (action == "insert") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				shared_ptr<RQL_Table> table;

				if (db_name == "") {
					table = conn->r()->table(ask("table_name"));
				}
				else {
					table = conn->r()->db(db_name)->table(ask("table_name"));
				}

				int count = atoi(ask("number of objects you want to insert").c_str());
				if (count == 1) {
					// insert one object
					responses = table->insert(RQL_Object(ask("key (string)"), RQL_String(ask("value (string)"))))->run();
				}
				else if (count > 1) {
					vector<shared_ptr<RQL_Datum>> objects = vector<shared_ptr<RQL_Datum>>();
					for (int i = 1; i <= count; ++i) {
						objects.push_back(make_shared<RQL_Datum>(RQL_Object(ask("key " + to_string(i) + " (string)"), RQL_String(ask("value " + to_string(i) + " (string)")))));
					}
					responses = table->insert(RQL_Array(objects))->run();
				}
			}
			else if (action == "between") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				shared_ptr<RQL_Table> table;

				if (db_name == "") {
					table = conn->r()->table(ask("table_name"));
				}
				else {
					table = conn->r()->db(db_name)->table(ask("table_name"));
				}
				responses = table->between(RQL_String(ask("id from (string)")), RQL_String(ask("id until (string)")))->run();
			}
			else if (action == "filter") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				shared_ptr<RQL_Table> table;

				if (db_name == "") {
					table = conn->r()->table(ask("table_name"));
				}
				else {
					table = conn->r()->db(db_name)->table(ask("table_name"));
				}
				responses = table->filter(RQL_Object(ask("key (string)"), RQL_String(ask("value (string)"))))->run();
			}
			else if (action == "get") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				shared_ptr<RQL_Table> table;

				if (db_name == "") {
					table = conn->r()->table(ask("table_name"));
				}
				else {
					table = conn->r()->db(db_name)->table(ask("table_name"));
				}
				responses = table->get(ask("id (string)"))->run();				
			}
			else if (action == "remove") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				shared_ptr<RQL_Table> table;

				if (db_name == "") {
					table = conn->r()->table(ask("table_name"));
				}
				else {
					table = conn->r()->db(db_name)->table(ask("table_name"));
				}
				string id = ask("id (leave empty for entire table)");
				if (id == "") {
					responses = table->remove()->run();
				}
				else {
					responses = table->get(id)->remove()->run();
				}
			}
			else if (action == "update") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				shared_ptr<RQL_Table> table;

				if (db_name == "") {
					table = conn->r()->table(ask("table_name"));
				}
				else {
					table = conn->r()->db(db_name)->table(ask("table_name"));
				}
				responses = table->get(ask("id (string)"))->update(RQL_Object(ask("key (string)"), RQL_String(ask("value (string)"))))->run();
			}
			else if (action == "limit") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				shared_ptr<RQL_Table> table;

				if (db_name == "") {
					table = conn->r()->table(ask("table_name"));
				}
				else {
					table = conn->r()->db(db_name)->table(ask("table_name"));
				}
				vector<shared_ptr<RQL_Ordering>> orderings = vector<shared_ptr<RQL_Ordering>>();
				orderings.push_back(RQL_Ordering::asc(ask("order_by key (string)")));
				responses = table->order_by(orderings)->limit(atoi(ask("limit").c_str()))->run();
			}
			else if (action == "order_by") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				shared_ptr<RQL_Table> table;

				if (db_name == "") {
					table = conn->r()->table(ask("table_name"));
				}
				else {
					table = conn->r()->db(db_name)->table(ask("table_name"));
				}
				vector<shared_ptr<RQL_Ordering>> orderings = vector<shared_ptr<RQL_Ordering>>();
				orderings.push_back(RQL_Ordering::asc(ask("order_by key (string)")));
				responses = table->order_by(orderings)->run();
			}
			else if (action == "skip") {
				string db_name = ask("db_name (leave empty for '" + conn->database + "')");
				shared_ptr<RQL_Table> table;

				if (db_name == "") {
					table = conn->r()->table(ask("table_name"));
				}
				else {
					table = conn->r()->db(db_name)->table(ask("table_name"));
				}
				vector<shared_ptr<RQL_Ordering>> orderings = vector<shared_ptr<RQL_Ordering>>();
				orderings.push_back(RQL_Ordering::asc(ask("order_by key (string)")));
				responses = table->order_by(orderings)->skip(atoi(ask("skip").c_str()))->run();
			}
			else {
				std::cout << "Invalid action." << endl;
			}

			for_each(responses.begin(), responses.end(), [] (shared_ptr<Response> response) {
				response->PrintDebugString();
			});
			
		}
		catch (runtime_error& e) {
			cerr << e.what() << endl;
		}

		action = "";
	}
	
	conn->close();

	return 0;
}
