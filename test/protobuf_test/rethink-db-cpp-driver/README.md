# RethinkDB C++ driver v0.1

> Disclaimer:  This project has been created for my C++ class at Charles University in Prague. It is not maintained anymore and may contain anti-patterns. It is not production-ready - use it at your own risk.

This project has been developed on (emulated) Windows 8 using Microsoft Visual Studio 2013 RC.

**Pull requests are welcomed!**

Functionality
-------------
Essential functionality has been implemented only.
    
	shared_ptr <connection> conn(new connection("host", "port", "default db", "auth_key"));

	conn->use("default_db"); // change default db

    conn->r()->db_create("my_db")->run(); // creating a db
	conn->r()->db_drop("my_db")->run(); // dropping a db
	conn->r()->db_list()->run(); // listing all dbs

	conn->r()->db("my_db")->table("my_table")->run(); // listing all entries in a table
    conn->r()->table_create("my_table")->run(); // creating a table in a default db
	conn->r()->db->("my_db")->table_drop("my_table")->run(); // dropping a table
	conn->r()->db->("my_db")->table_list()->run(); // listing all tables

	conn->r()->db(db_name)->table("my_table")->insert(RQL_Object("key", RQL_String("value")))->run(); // inserting an object
	conn->r()->db(db_name)->table("my_table")->update(RQL_Object("key", RQL_String("value")))->run(); // updating an object

	conn->r()->db(db_name)->table("my_table")->get("my_id")->run(); // getting an object by its primary key
	conn->r()->db(db_name)->table("my_table")->between("from", "to")->run(); // getting an object inbetween given primary keys

	conn->r()->db(db_name)->table("my_table")->get("my_id")->remove()->run(); // removing an object by its primary key
    conn->r()->db(db_name)->table("my_table")->filter(RQL_Object("my_key", RQL_String("value")))->remove()->run(); // removing all filtered objects
	
	// filter all results, order them, set an offset and a limit
	conn->r()->db(db_name)->table("my_table")->filter(RQL_Object("my_key", RQL_String("value")))->order_by(RQL_Ordering::asc("my_key"))->skip(3)->limit(10)->run();
	
See `example.cpp` for more.

Project structure
-------------------------

| File                                  | Description |
| ------------------------------------- | ----------- |
| LICENSE                               | 
| LICENSE.txt                           | 
| README.md                             | 
| connection.cpp                        | Object for maintaining a single connection to RDB
| connection.hpp                        | 
| example.cpp                           | Example usage of the driver, fully compilable
| exception.hpp                         | Definitions of exceptions
| include                               | Protobuf sources
| lib                                   | Protobuf libraries compiled for Windows 8 (please ignore if not using Windows 8)
| proto                                 | RethinkBD Protobuf
| rethink_db.hpp                        | Meta file for inclusion in your project
| rethink_db_cpp_driver.opensdf         | MS VS 2013 Project file
| rethink_db_cpp_driver.sdf             | MS VS 2013 Project file
| rethink_db_cpp_driver.sln             | MS VS 2013 Project file
| rethink_db_cpp_driver.v12.suo         | MS VS 2013 Project file
| rethink_db_cpp_driver.vcxproj         | MS VS 2013 Project file
| rethink_db_cpp_driver.vcxproj.filters | MS VS 2013 Project file
| rethink_db_cpp_driver.vcxproj.user    | MS VS 2013 Project file
| rql.cpp                               | Implementation of RQL data structures in C++
| rql.hpp                               | Implementation of RQL data structures in C++

Usage instructions
------------------

1. checkout the repository
2. make sure you have boost and protobuf installed - if you are on Windows 8 you can make use of the `lib/` and `include/` directories
4. `#include "rethinkdb.hpp"` and you are set!
5. see `example.cpp` for more

License
-------

    Released under MIT license.

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
