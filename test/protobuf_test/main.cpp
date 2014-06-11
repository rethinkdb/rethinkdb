//
//  main.cpp
//  protobuf_test
//
//  Created by Karl Kuehn on 5/21/14.
//  Copyright (c) 2014 RethinkDB. All rights reserved.
//

#include <stdlib.h>
#include <getopt.h>
#include <iostream>
#include <vector>

#include "rethink_db.hpp"

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT "28015"
#define DEFAULT_DB_NAME "protobuf_test_db"

#define TABLE_A_NAME "table_a"
#define TABLE_A_KEY "data"
#define RECORDS_TO_CREATE 10

using namespace std;
using namespace com::rethinkdb;
using namespace com::rethinkdb::driver;

int main(int argc, char * const argv[]) {
	// -- parse input for connection information
	
	// - defaults
	
	const char * host		= DEFAULT_HOST;
	const char * port		= DEFAULT_PORT;
	const char * db_name	= DEFAULT_DB_NAME;
	const char * auth_key	= "";
	
	// - ENV variables
	
	if (getenv("HOST") != NULL) {
		host = getenv("HOST");
	}
	
	if (getenv("PORT") != NULL) {
		port = getenv("PORT");
	}
	
	if (getenv("DB_NAME") != NULL) {
		db_name = getenv("DB_NAME");
	}
	
	// - command-line arguments
	
	static struct option long_options[] = {
		{"host",	required_argument,	0,	'n' },
		{"port",	required_argument,	0,	'p' },
		{"db",		required_argument,	0,	'd' },
		{"authkey",	required_argument,	0,	'k' },
		{0,			0,					0,	0 }
	};
	int option_index = 0;
	int optionChosen;
	while ((optionChosen = getopt_long(argc, argv, "n:p:d:k:", long_options, &option_index)) != -1) {
		switch(optionChosen) {
			case 'n':
				host = optarg;
				break;
			case 'p':
				port = optarg;
				break;
			case 'd':
				db_name = optarg;
				break;
			case 'k':
				auth_key = optarg;
				break;
		}
	}
	
	// - summarize
	
	string auth_key_message = auth_key;
	if (strlen(auth_key) == 0) {
		auth_key_message = "(none)";
	}
	std::cout << "Testing using" << endl << "    server:\t" << host << ":" << port << endl << "    db name:\t" << db_name << endl << "    authkey:\t" << auth_key_message << endl;
	
	// -- open connection
	
	shared_ptr <connection> dbConnection(new connection(host, port, db_name, auth_key));
	if (dbConnection->connect() == false) {
		std::cerr << "Failed opening connection" << endl;
		return 1;
	}
	
	// -- run tests
	
	vector<shared_ptr<Response>> responseAtoms = vector<shared_ptr<Response>>();
	int passedTests	= 0;
	int failedTests	= 0;
	
	bool failedThisTest;
	
	// - list databases
	
	try {
		responseAtoms = dbConnection->r()->db_list()->run();
		failedThisTest = false;
		
		// remove the target database if it is there
		
		for_each(responseAtoms.begin(), responseAtoms.end(), [&] (shared_ptr<Response> responseAtom) {
			if (responseAtom->type() != Response_ResponseType_SUCCESS_ATOM) {
				std::cerr << "Failed listing databases, got a non-success response" << endl;
			} else {
				for(int i = 0; i < responseAtom->response_size(); i++) {
					for (int j = 0; j < responseAtom->response(i).r_array_size(); j++) {
						if (responseAtom->response(i).r_array(j).r_str() == db_name) {
							dbConnection->r()->db_drop(db_name)->run();
							std::cout << "Removed the " << db_name << " database" << endl;
						}
					}
				}
			}
		});
		
		if (failedThisTest == false) {
			passedTests++;
			std::cout << "Sucessfully listed databases" << endl;
		} else {
			failedTests++;
		}
	} catch (runtime_error& e) {
		failedTests++;
		std::cerr << "Failed listing databases: " << e.what() << endl;
	}
	
	// - create the database

	try {
		responseAtoms = dbConnection->r()->db_create(db_name)->run();
		if (responseAtoms.size() != 1) {
			failedTests++;
			std::cerr << "Failed creating database: looking for 1 response, got " << responseAtoms.size() << endl;
		
		} else if (responseAtoms.at(0)->type() != Response_ResponseType_SUCCESS_ATOM) {
			failedTests++;
			std::cerr << "Failed creating database: did not get a sucess message, got: " << responseAtoms.at(0)->Utf8DebugString() << endl;
		
		} else {
			passedTests++;
			std::cout << "Sucessfully created database: " << db_name << endl;
		}
	} catch (runtime_error& e) {
		failedTests++;
		std::cerr << "Failed creating database: " << e.what() << endl;
	}
	
	// - create table_a
	
	try {
		responseAtoms = dbConnection->r()->db(db_name)->table_create(TABLE_A_NAME)->run();
		if (responseAtoms.size() != 1) {
			failedTests++;
			std::cerr << "Failed creating table " << TABLE_A_NAME << ": looking for 1 response, got " << responseAtoms.size() << endl;
			
		} else if (responseAtoms.at(0)->type() != Response_ResponseType_SUCCESS_ATOM) {
			failedTests++;
			std::cerr << "Failed creating table " << TABLE_A_NAME << ": did not get a sucess message, got: " << responseAtoms.at(0)->Utf8DebugString() << endl;
			
		} else {
			passedTests++;
			std::cout << "Sucessfully created table: " << TABLE_A_NAME << endl;
		}
	} catch (runtime_error& e) {
		failedTests++;
		std::cerr << "Failed creating table " << TABLE_A_NAME << ": " << e.what() << endl;
	}
	
	// - fill in some data

	try {
		failedThisTest = false;
		for (int i = 1; i < RECORDS_TO_CREATE + 1; i++) {
			responseAtoms = dbConnection->r()->db(db_name)->table(TABLE_A_NAME)->insert(RQL_Object(TABLE_A_KEY, RQL_Number(i)))->run();
			if (responseAtoms.size() != 1) {
				failedThisTest = true;
				std::cerr << "Failed filling table " << TABLE_A_NAME << ": looking for 1 response, got " << responseAtoms.size() << endl;
				break;
				
			} else if (responseAtoms.at(0)->type() != Response_ResponseType_SUCCESS_ATOM) {
				failedThisTest = true;
				std::cerr << "Failed filling table " << TABLE_A_NAME << ": did not get a sucess message, got: " << responseAtoms.at(0)->Utf8DebugString() << endl;
				break;
			}
		}
		if (failedThisTest == false) {
			passedTests++;
			std::cout << "Sucessfully filled table " << TABLE_A_NAME << endl;
		} else {
			failedTests++;
		}
	} catch (runtime_error& e) {
		failedTests++;
		std::cerr << "Failed filling table " << TABLE_A_NAME << ": " << e.what() << endl;
	}
	
	// - verify the data
	
	failedThisTest = false;
	for (int i = 1; i < RECORDS_TO_CREATE + 1; i++) {
		try {
			responseAtoms = dbConnection->r()->db(db_name)->table(TABLE_A_NAME)->filter(RQL_Object(TABLE_A_KEY, RQL_Number(i)))->run();
			for_each(responseAtoms.begin(), responseAtoms.end(), [&] (shared_ptr<Response> responseAtom) {
				if (responseAtom->type() != Response_ResponseType_SUCCESS_SEQUENCE) {
					failedThisTest = true;
					std::cerr << "Failed getting record " << TABLE_A_KEY << endl;
				} else {
					if (responseAtom->response_size() != 1) {
						failedThisTest = true;
						std::cerr << "Failed getting record " << TABLE_A_KEY << ", the response size was not 1 (" << responseAtom->response_size() << ")" << endl;
					} else if (responseAtom->response(0).r_object_size() != 2) {
						failedThisTest = true;
						std::cerr << "Failed getting record " << TABLE_A_KEY << ", the response object size was not 2 (" << responseAtom->response(0).r_object_size() << ")" << endl;
					} else {
						for (int j = 0; j < responseAtom->response(0).r_object_size(); j++) {
							if (responseAtom->response(0).r_object(j).key() == TABLE_A_KEY && responseAtom->response(0).r_object(j).val().r_num() != i) {
								failedThisTest = true;
								std::cerr << "Failed getting record " << TABLE_A_KEY << ", the response was not " << i << " (" << responseAtom->response(0).r_object(j).val().r_num() << ")" << endl;
							}
						}
					}
				}
			});
			
		} catch (runtime_error& e) {
			failedThisTest = true;
			std::cerr << "Failed getting record " << TABLE_A_KEY << ": " << e.what() << endl;
			break;
		}
	}
	if (failedThisTest == false) {
		passedTests++;
		std::cout << "Sucessfully got all records from " << TABLE_A_NAME << endl;
	} else {
		failedTests++;
	}
	
	
	// - delete the database
	
	try {
		responseAtoms = dbConnection->r()->db_drop(db_name)->run();
		if (responseAtoms.size() != 1) {
			failedTests++;
			std::cerr << "Failed dropping database: looking for 1 response, got " << responseAtoms.size() << endl;
			
		} else if (responseAtoms.at(0)->type() != Response_ResponseType_SUCCESS_ATOM) {
			failedTests++;
			std::cerr << "Failed dropping database: did not get a sucess message, got: " << responseAtoms.at(0)->Utf8DebugString() << endl;
			
		} else {
			passedTests++;
			std::cout << "Sucessfully dropped database: " << db_name << endl;
		}
	} catch (runtime_error& e) {
		failedTests++;
		std::cerr << "Failed dropping database " << db_name << ": " << e.what() << endl;
	}
	
	// - handle an error
	
	try {
		responseAtoms = dbConnection->r()->db_drop(db_name)->run();
		failedTests++;
		std::cerr << "Failed while expecting an error dropping a dead database" << endl;
	} catch (runtime_error& e) {
		passedTests++;
		std::cerr << "Sucessfully got an error when expected" << endl;
	}
	
	// -- return
	
	if (failedTests != 0) {
		std::cerr << "Test failed " << failedTests << " out of " << failedTests + passedTests << " subtests" << endl;
		return 1;
	}
	std::cout << "Passed all " << passedTests << " subtests" << endl;
	return 0;
}

