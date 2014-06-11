#ifndef RETHINK_DB_DRIVER_RQL
#define RETHINK_DB_DRIVER_RQL

#include "proto/rdb_protocol/ql2.pb.h"
#include <vector>

using namespace std;
using namespace com::rethinkdb;

namespace com {
	namespace rethinkdb {
		namespace driver {
			
			class connection;
			class RQL_Array;
			class RQL_Object;
			class RQL_String;
			class RQL_Database;
			class RQL_Table;
			class RQL { // "Top" according to photobuf data structure specification
			public:			

				RQL() : term(Term()) {};

				vector<shared_ptr<Response>> run();				

				/* -------------------------------------------------------------------- */

				shared_ptr<RQL_Database> db(const RQL_String& db_name);
				shared_ptr<RQL_Database> db(const string& db_name);
				shared_ptr<RQL_Array> db_list();
				shared_ptr<RQL_Object> db_create(const RQL_String& db_name);
				shared_ptr<RQL_Object> db_create(const string& db_name);
				shared_ptr<RQL_Object> db_drop(const RQL_String& db_name);
				shared_ptr<RQL_Object> db_drop(const string& db_name);

				/* -------------------------------------------------------------------- */

				virtual shared_ptr<RQL_Table> table(const RQL_String& table_name);
				virtual shared_ptr<RQL_Table> table(const string& table_name);
				virtual shared_ptr<RQL_Object> table_create(const RQL_String& table_name);
				virtual shared_ptr<RQL_Object> table_create(const string& table_name);
				virtual shared_ptr<RQL_Array> table_drop(const RQL_String& table_name);
				virtual shared_ptr<RQL_Array> table_drop(const string& table_name);
				virtual shared_ptr<RQL_Array> table_list();

				/* -------------------------------------------------------------------- */

				Term term;
				shared_ptr <connection> conn;

			protected:				

				void check_response(shared_ptr<Response> response);

			};

			class RQL_Datum : public virtual RQL {};

			class RQL_String : public RQL_Datum {
			public:
				RQL_String(const string& str) {
					term.set_type(Term::TermType::Term_TermType_DATUM);
					term.mutable_datum()->set_type(Datum::DatumType::Datum_DatumType_R_STR);
					term.mutable_datum()->set_r_str(str);
				}
			};

			class RQL_Number : public RQL_Datum {
			public:
				RQL_Number(double number) {
					term.set_type(Term::TermType::Term_TermType_DATUM);
					term.mutable_datum()->set_type(Datum::DatumType::Datum_DatumType_R_NUM);
					term.mutable_datum()->set_r_num(number);
				}
			};

			class RQL_Object : public RQL_Datum {
			public:
				RQL_Object() {};
				RQL_Object(const string& key, const RQL_Datum& value) {
					term.set_type(Term::TermType::Term_TermType_DATUM);
					term.mutable_datum()->set_type(Datum::DatumType::Datum_DatumType_R_OBJECT);
					Datum_AssocPair* ptr = term.mutable_datum()->add_r_object();
					ptr->set_key(key);
					*(ptr->mutable_val()) = value.term.datum();
				}
			};

			class RQL_Function : public RQL {};
			class RQL_Ordering : public RQL {
			public:
				static shared_ptr<RQL_Ordering> asc(const RQL_String& str) {
					shared_ptr<RQL_Ordering> ordering(new RQL_Ordering());
					ordering->term.set_type(Term::TermType::Term_TermType_ASC);
					*(ordering->term.add_args()) = str.term;
					return ordering;
				}

				static shared_ptr<RQL_Ordering> asc(const string& str) {
					return asc(RQL_String(str));
				}

				static shared_ptr<RQL_Ordering> desc(const RQL_String& str) {
					shared_ptr<RQL_Ordering> ordering(new RQL_Ordering());
					ordering->term.set_type(Term::TermType::Term_TermType_DESC);
					*(ordering->term.add_args()) = str.term;
					return ordering;
				}

				static shared_ptr<RQL_Ordering> desc(const string& str) {
					return desc(RQL_String(str));
				}
			};
			class RQL_Pathspec : public RQL {};

			class RQL_Sequence : public virtual RQL {
			public:

				shared_ptr<RQL_Sequence> filter(const RQL_Function& object) {
					shared_ptr<RQL_Sequence> sequence(new RQL_Sequence());
					sequence->term.set_type(Term::TermType::Term_TermType_FILTER);
					*(sequence->term.add_args()) = this->term;
					*(sequence->term.add_args()) = object.term;
					sequence->conn = this->conn;
					return sequence;
				}

				shared_ptr<RQL_Sequence> filter(const RQL_Object& object) {
					shared_ptr<RQL_Sequence> sequence(new RQL_Sequence());
					sequence->term.set_type(Term::TermType::Term_TermType_FILTER);
					*(sequence->term.add_args()) = this->term;
					*(sequence->term.add_args()) = object.term;
					sequence->conn = this->conn;
					return sequence;
				}

				shared_ptr<RQL_Sequence> order_by(const vector<shared_ptr<RQL_Ordering>>& orderings) {
					shared_ptr<RQL_Sequence> sequence(new RQL_Sequence());
					sequence->term.set_type(Term::TermType::Term_TermType_ORDERBY);
					*(sequence->term.add_args()) = this->term;
					for_each(orderings.begin(), orderings.end(), [sequence](shared_ptr<RQL_Ordering> ordering) {
						*(sequence->term.add_args()) = ordering->term;
					});
					sequence->conn = this->conn;
					return sequence;
				}

				shared_ptr<RQL_Sequence> order_by(const RQL_Ordering& ordering) {
					shared_ptr<RQL_Sequence> sequence(new RQL_Sequence());
					sequence->term.set_type(Term::TermType::Term_TermType_ORDERBY);
					*(sequence->term.add_args()) = this->term;
					*(sequence->term.add_args()) = ordering.term;
					sequence->conn = this->conn;
					return sequence;
				}

				shared_ptr<RQL_Sequence> skip(const RQL_Number& number) {
					shared_ptr<RQL_Sequence> sequence(new RQL_Sequence());
					sequence->term.set_type(Term::TermType::Term_TermType_SKIP);
					*(sequence->term.add_args()) = this->term;
					*(sequence->term.add_args()) = number.term;
					sequence->conn = this->conn;
					return sequence;
				}

				shared_ptr<RQL_Sequence> skip(size_t number) {
					return skip(RQL_Number((double)number));
				}

				shared_ptr<RQL_Sequence> limit(const RQL_Number& number) {
					shared_ptr<RQL_Sequence> sequence(new RQL_Sequence());
					sequence->term.set_type(Term::TermType::Term_TermType_LIMIT);
					*(sequence->term.add_args()) = this->term;
					*(sequence->term.add_args()) = number.term;
					sequence->conn = this->conn;
					return sequence;
				}

				shared_ptr<RQL_Sequence> limit(size_t number) {
					return limit(RQL_Number((double)number));
				}
			};
			class RQL_Stream : public RQL_Sequence {};	

			class RQL_Stream_Selection : public RQL_Stream {
			public:
				shared_ptr<RQL_Stream_Selection> between(const RQL_Datum& d1, const RQL_Datum& d2) {
					shared_ptr<RQL_Stream_Selection> selection(new RQL_Stream_Selection());
					selection->term.set_type(Term::TermType::Term_TermType_BETWEEN);
					*(selection->term.add_args()) = this->term;
					*(selection->term.add_args()) = d1.term;
					*(selection->term.add_args()) = d2.term;
					selection->conn = this->conn;
					return selection;
				}

				// "delete" is a reserved word in C++ :-(
				shared_ptr<RQL_Object> remove() {
					shared_ptr<RQL_Object> object(new RQL_Object());
					object->term.set_type(Term::TermType::Term_TermType_DELETE);
					*(object->term.add_args()) = this->term;
					object->conn = this->conn;
					return object;
				}

				shared_ptr<RQL_Object> update(const RQL_Function& o) {
					shared_ptr<RQL_Object> object(new RQL_Object());
					object->term.set_type(Term::TermType::Term_TermType_UPDATE);
					*(object->term.add_args()) = this->term;
					*(object->term.add_args()) = o.term;
					object->conn = this->conn;
					return object;
				}

				shared_ptr<RQL_Object> update(const RQL_Object& o) {
					shared_ptr<RQL_Object> object(new RQL_Object());
					object->term.set_type(Term::TermType::Term_TermType_UPDATE);
					*(object->term.add_args()) = this->term;
					*(object->term.add_args()) = o.term;
					object->conn = this->conn;
					return object;
				}
			};			

			class RQL_Null : public RQL_Datum {};
			class RQL_Bool : public RQL_Datum {
			public:
				RQL_Bool(bool b) {
					term.set_type(Term::TermType::Term_TermType_DATUM);
					term.mutable_datum()->set_type(Datum::DatumType::Datum_DatumType_R_BOOL);
					term.mutable_datum()->set_r_bool(b);
				}
			};			

			class RQL_Single_Selection : public RQL_Object{
			public:
				// "delete" is a reserved word in C++ :-(
				shared_ptr<RQL_Object> remove() {
					shared_ptr<RQL_Object> object(new RQL_Object());
					object->term.set_type(Term::TermType::Term_TermType_DELETE);
					*(object->term.add_args()) = this->term;
					object->conn = this->conn;
					return object;
				}

				shared_ptr<RQL_Object> update(const RQL_Function& o) {
					shared_ptr<RQL_Object> object(new RQL_Object());
					object->term.set_type(Term::TermType::Term_TermType_UPDATE);
					*(object->term.add_args()) = this->term;
					*(object->term.add_args()) = o.term;
					object->conn = this->conn;
					return object;
				}

				shared_ptr<RQL_Object> update(const RQL_Object& o) {
					shared_ptr<RQL_Object> object(new RQL_Object());
					object->term.set_type(Term::TermType::Term_TermType_UPDATE);
					*(object->term.add_args()) = this->term;
					*(object->term.add_args()) = o.term;
					object->conn = this->conn;
					return object;
				}
			};

			class RQL_Table : public RQL_Stream_Selection {
			public:

				shared_ptr<RQL_Single_Selection> get(const RQL_String& key) {
					shared_ptr<RQL_Single_Selection> object(new RQL_Single_Selection());
					object->term.set_type(Term::TermType::Term_TermType_GET);
					*(object->term.add_args()) = this->term;
					*(object->term.add_args()) = key.term;
					object->conn = this->conn;
					return object;
				}

				shared_ptr<RQL_Single_Selection> get(const string& key) {
					return get(RQL_String(key));
				}

				shared_ptr<RQL_Single_Selection> get(const RQL_Number& key) {
					shared_ptr<RQL_Single_Selection> object(new RQL_Single_Selection());
					object->term.set_type(Term::TermType::Term_TermType_GET);
					*(object->term.add_args()) = this->term;
					*(object->term.add_args()) = key.term;
					object->conn = this->conn;
					return object;
				}

				shared_ptr<RQL_Single_Selection> get(double key) {
					return get(RQL_Number(key));
				}

				shared_ptr<RQL_Object> insert(const RQL_Object& o) {
					shared_ptr<RQL_Object> object(new RQL_Object());
					object->term.set_type(Term::TermType::Term_TermType_INSERT);
					*(object->term.add_args()) = this->term;
					*(object->term.add_args()) = o.term;
					object->conn = this->conn;
					return object;
				}

				shared_ptr<RQL_Object> insert(const RQL_Sequence& sequence) {
					shared_ptr<RQL_Object> object(new RQL_Object());
					object->term.set_type(Term::TermType::Term_TermType_INSERT);
					*(object->term.add_args()) = this->term;
					*(object->term.add_args()) = sequence.term;
					object->conn = this->conn;
					return object;
				}				
			};

			class RQL_Array : public RQL_Datum, public RQL_Sequence {
			public:
				RQL_Array() {}

				RQL_Array(const vector < shared_ptr < RQL_Datum >> &params) {
					term.set_type(Term::TermType::Term_TermType_MAKE_ARRAY);
					for_each(params.begin(), params.end(), [this](shared_ptr<RQL_Datum> datum) {
						*(term.add_args()) = datum->term;
					});
				}
			};

			class RQL_Database : public RQL {
			public:
				shared_ptr<RQL_Table> table(const RQL_String& table_name) {
					shared_ptr<RQL_Table> object(new RQL_Table());
					object->term.set_type(Term::TermType::Term_TermType_TABLE);
					*(object->term.add_args()) = this->term;
					*(object->term.add_args()) = table_name.term;
					object->conn = this->conn;
					return object;
				}

				shared_ptr<RQL_Table> table(const string& table_name) {
					return table(RQL_String(table_name));
				}

				shared_ptr<RQL_Object> table_create(const RQL_String& table_name) {
					shared_ptr<RQL_Object> object(new RQL_Object());
					object->term.set_type(Term::TermType::Term_TermType_TABLE_CREATE);
					*(object->term.add_args()) = this->term;
					*(object->term.add_args()) = table_name.term;
					object->conn = this->conn;
					return object;
				}

				shared_ptr<RQL_Object> table_create(const string& table_name) {
					return table_create(RQL_String(table_name));
				}

				shared_ptr<RQL_Array> table_drop(const RQL_String& table_name) {
					shared_ptr<RQL_Array> array(new RQL_Array());
					array->term.set_type(Term::TermType::Term_TermType_TABLE_DROP);
					*(array->term.add_args()) = this->term;
					*(array->term.add_args()) = table_name.term;
					array->conn = this->conn;
					return array;
				}

				shared_ptr<RQL_Array> table_drop(const string& table_name) {
					return table_drop(RQL_String(table_name));
				}

				shared_ptr<RQL_Array> table_list() {
					shared_ptr<RQL_Array> array(new RQL_Array());
					array->term.set_type(Term::TermType::Term_TermType_TABLE_LIST);
					*(array->term.add_args()) = this->term;
					array->conn = this->conn;
					return array;
				}
			};
		}
	}
}

#endif
