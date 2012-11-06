
reql_docs = [
  # Manipulating databases
  db_admin:
    name: 'Manipulating databases'
    description: 'These commands allow database manipulation.'
    commands: [
      # Create database
      db_create:
        description: 'Create a database. A RethinkDB database is a collection of tables, similar to relational databases.'
        parent: 'r'
        returns: null
        langs:
          js:
            name: 'dbCreate'
            body: 'dbName'
            examples: [
              description: "Create a database named 'superheroes'"
              code: "r.dbCreate('superheroes').run()"
            ]
          py:
            name: 'db_create'
            body: 'db_name'
            examples: [
              description: "Create a database named 'superheroes'"
              code: "r.db_create('superheroes').run()"
            ]
          ry:
            name: 'db_create'
            body: 'db_name'
            examples: [
              description: "Create a database named 'superheroes'"
              code: "r.db_create('superheroes').run"
            ]
      ,
      # Drop database
      db_drop:
        description: 'Drop a database. The database, all its tables, and corresponding data will be deleted.'
        parent: 'r'
        returns: null
        langs:
          js:
            name: 'dbDrop'
            body: 'dbName'
            examples: [
              description: "Drop a database named 'superheroes'"
              code: "r.dbDrop('superheroes').run()"
            ]
          py:
            name: 'db_drop'
            body: 'db_name'
            examples: [
              description: "Drop a database named 'superheroes'"
              code: "r.db_drop('superheroes').run()"
            ]
          ry:
            name: 'db_drop'
            body: 'db_name'
            examples: [
              description: "Drop a database named 'superheroes'"
              code: "r.db_drop('superheroes').run"
            ]
      ,
      # List database
      db_list:
        description: 'List all database names in the system.'
        parent: 'r'
        returns: null
        langs:
          js:
            name: 'dbList'
            body: ''
            examples: [
              description: "List all databases"
              code: "r.dbList().run()"
            ]
          py:
            name: 'db_list'
            body: ''
            examples: [
              description: "List all databases"
              code: "r.db_list().run()"
            ]
          ry:
            name: 'db_list'
            body: ''
            examples: [
              description: "List all databases"
              code: "r.db_list.run"
            ]
    ]
  # Manipulating tables
  table_admin:
    name: 'Manipulating tables'
    description: 'These commands allow table manipulation.'
    commands: [
      # Create database
      table_create:
        description: 'Create a table. A RethinkDB table is a collection of JSON documents.'
        parent: 'db'
        returns: null
        langs:
          js:
            name: 'tableCreate'
            body: 'tableName'
            examples: [
              description: "Create a table named 'marvel'."
              code: "r.tableCreate('marvel').run()"
            ]
          py:
            name: 'table_create'
            body: 'table_name'
            examples: [
              description: "Create a table named 'marvel'"
              code: "r.table_create('marvel').run()"
            ]
          ry:
            name: 'table_create'
            body: 'table_name'
            examples: [
              description: "Create a table named 'marvel'"
              code: "r.table_create('marvel').run"
            ]
      ,
      # Drop table
      table_drop:
        description: 'Drop a table. The table and all its rows will be deleted.'
        parent: 'db'
        returns: null
        langs:
          js:
            name: 'tableDrop'
            body: 'tableName'
            examples: [
              description: "Drop a table named 'marvel'"
              code: "r.tableDrop('marvel').run()"
            ]
          py:
            name: 'table_drop'
            body: 'table_name'
            examples: [
              description: "Drop a table named 'marvel'"
              code: "r.table_drop('marvel').run()"
            ]
          ry:
            name: 'table_drop'
            body: 'table_name'
            examples: [
              description: "Drop a table named 'marvel'"
              code: "r.table_drop('marvel').run"
            ]
      ,
      # List tables
      table_list:
        description: 'List all table names in the system.'
        parent: 'db'
        returns: null
        langs:
          js:
            name: 'tableList'
            body: ''
            examples: [
              description: "List all tables"
              code: "r.tableList().run()"
            ]
          py:
            name: 'table_list'
            body: ''
            examples: [
              description: "List all tables"
              code: "r.table_list().run()"
            ]
          ry:
            name: 'table_list'
            body: ''
            examples: [
              description: "List all tables"
              code: "r.table_list().run"
            ]
    ]
  # Manipulating tables
  writing_data:
    name: 'Writing data'
    description: 'These commands allow inserting, deleting, and updating data.'
    commands: [
      # Insert
      insert:
        description: 'Insert JSON documents into a table. Accepts a single JSON document or an array of documents.'
        parent: 'table'
        returns: null
        langs:
          js:
            name: 'insert'
            body: 'json | [json]'
            examples: [
              description: "Insert a row into a table named 'marvel'."
              code: "r.table('marvel').insert({ superhero: 'Iron Man', superpower: 'Arc Reactor' }).run()"
            ,
              description: "Insert multiple rows into a table named 'marvel'."
              code: '''
                    r.table('marvel').insert([{ superhero: 'Wolverine', superpower: 'Adamantium' },
                                              { superhero: 'Spiderman', superpower: 'spidy sense' }])
                                     .run()
                    '''
            ]
          py:
            name: 'insert'
            body: 'json | [json]'
            examples: [
              description: "Insert a row into a table named 'marvel'."
              code: "r.table('marvel').insert({ 'superhero': 'Iron Man', 'superpower': 'Arc Reactor' }).run()"
            ,
              description: "Insert multiple rows into a table named 'marvel'."
              code: '''
                    r.table('marvel').insert([{ 'superhero': 'Wolverine', 'superpower': 'Adamantium' },
                                              { 'superhero': 'Spiderman', 'superpower': 'spidy sense' }])
                                     .run()
                    '''
            ]
          ry:
            name: 'insert'
            body: 'json | [json]'
            examples: [
              description: "Insert a row into a table named 'marvel'."
              code: "r.table('marvel').insert({ 'superhero'=>'Iron Man', 'superpower'=>'Arc Reactor' }).run"
            ,
              description: "Insert multiple rows into a table named 'marvel'."
              code: '''
                    r.table('marvel').insert([{ 'superhero'=>'Wolverine', 'superpower'=>'Adamantium' },
                                              { 'superhero'=>'Spiderman', 'superpower'=>'spidy sense' }])
                                     .run
                    '''
            ]
      ,
      # Update
      update:
        description: 'Update JSON documents in a table. Accepts a JSON document, a ReQL expression, or a combination of the two.'
        parent: 'selection'
        returns: null
        langs:
          js:
            name: 'update'
            body: 'json | expr'
            examples: [
              description: "Update Superman's age to 30. If attribute 'age' doesn't exist, adds it to the document"
              code: "r.table('marvel').get('superman').update({ age: 30 }).run()"
            ,
              description: "Increment every superhero's age. If age doesn't exist, throws an error"
              code: "r.table('marvel').update({ age: r('age').add(1) }).run()"
            ]
          py:
            name: 'update'
            body: 'json | expr'
            examples: [
              description: "Update Superman's age to 30. If attribute 'age' doesn't exist, adds it to the document"
              code: "r.table('marvel').get('superman').update({ 'age': 30 }).run()"
            ,
              description: "Increment every superhero's age. If age doesn't exist, throws an error"
              code: "r.table('marvel').update({ 'age': r['age'] + 1 }).run()"
            ]
          ry:
            name: 'update'
            body: 'json | expr'
            examples: [
              description: "Update Superman's age to 30. If attribute 'age' doesn't exist, adds it to the document"
              code: "r.table('marvel').get('superman').update({ 'age'=>30 }).run"
            ,
              description: "Increment every superhero's age. If age doesn't exist, throws an error"
              code: "r.table('marvel').update{ |hero| { 'age'=>hero['age'] + 1 } }.run"
            ]
      # Replace
      replace:
        description:
          '''
          Replace documents in a table. Accepts a JSON document or a ReQL expression, and replaces the original
          document with the new one. The new document must have the same primary key as the original document.
          '''
        parent: 'selection'
        returns: null
        langs:
          js:
            name: 'replace'
            body: 'json | expr'
            examples: [
              description: "Remove all existing attributes from Superman's document, and add an attribute 'age'"
              code: "r.table('marvel').get('superman').replace({ id: 'superman', age: 30 }).run()"
            ]
          py:
            name: 'replace'
            body: 'json | expr'
            examples: [
              description: "Remove all existing attributes from Superman's document, and add an attribute 'age'"
              code: "r.table('marvel').get('superman').replace({ 'id': 'superman', 'age': 30 }).run()"
            ]
          ry:
            name: 'replace'
            body: 'json | expr'
            examples: [
              description: "Remove all existing attributes from Superman's document, and add an attribute 'age'"
              code: "r.table('marvel').get('superman').replace({ 'id'=>'superman', 'age'=>30 }).run"
            ]
      # Delete
      delete:
        description: 'Delete one or more documents from a table.'
        parent: 'selection'
        returns: null
        langs:
          js:
            name: 'del'
            body: ''
            examples: [
              description: "Delete superman from the database"
              code: "r.table('marvel').get('superman').del().run()"
            ,
              description: "Delete every document from the table 'marvel'"
              code: "r.table('marvel').del().run()"
            ]
          py:
            name: 'delete'
            body: ''
            examples: [
              description: "Delete superman from the database"
              code: "r.table('marvel').get('superman').delete().run()"
            ,
              description: "Delete every document from the table 'marvel'"
              code: "r.table('marvel').delete().run()"
            ]
          ry:
            name: 'delete'
            body: ''
            examples: [
              description: "Delete superman from the database"
              code: "r.table('marvel').get('superman').delete.run"
            ,
              description: "Delete every document from the table 'marvel'"
              code: "r.table('marvel').delete.run"
            ]
    ]
  # Selecting data
  select:
    name: 'Selecting data'
    description: 'These commands allow searching for data in the database.'
    commands: [
      # Access documents in a table
      table:
        description: "Select all documents in a table. This command can be chained with other commands to do further processing on the data."
        parent: 'db'
        returns: 'selection'
        langs:
          js:
            name: 'table'
            body: 'name'
            examples: [
              description: "Return all documents in the table 'marvel'."
              code: "r.table('marvel').run()"
            ]
          py:
            name: 'table'
            body: 'name'
            examples: [
              description: "Return all documents in the table 'marvel'."
              code: "r.table('marvel').run()"
            ]
          ry:
            name: 'table'
            body: 'name'
            examples: [
              description: "Return all documents in the table 'marvel'."
              code: "r.table('marvel').run"
            ]
      # Get a document
      get:
        description: "Get a document by primary key. If the primary key is 'id', the second argument is optional."
        parent: 'table'
        returns: 'json'
        langs:
          js:
            name: 'get'
            body: 'key [, key_attribute]'
            examples: [
              description: "Find a document with the primary key 'superman'. Expects the primary key to be in an attribute 'id'"
              code: "r.table('marvel').get('superman').run()"
            ,
              description: "Find a document with the primary key 'superman'. Expects the primary key to be in an attribute 'name'"
              code: "r.table('marvel').get('superman', 'name').run()"
            ]
          py:
            name: 'get'
            body: 'key [, key_attribute]'
            examples: [
              description: "Find a document with the primary key 'superman'. Expects the primary key to be in an attribute 'id'"
              code: "r.table('marvel').get('superman').run()"
            ,
              description: "Find a document with the primary key 'superman'. Expects the primary key to be in an attribute 'name'"
              code: "r.table('marvel').get('superman', 'name').run()"
            ]
          ry:
            name: 'get'
            body: 'key [, key_attribute]'
            examples: [
              description: "Find a document with the primary key 'superman'. Expects the primary key to be in an attribute 'id'"
              code: "r.table('marvel').get('superman').run"
            ,
              description: "Find a document with the primary key 'superman'. Expects the primary key to be in an attribute 'name'"
              code: "r.table('marvel').get('superman', 'name').run"
            ]
      # Get a range of documents
      between:
        description: "Get all documents between two primary keys (both keys are inclusive). If the primary key is 'id', the last argument can be omitted."
        parent: 'selection'
        returns: 'selection'
        langs:
          js:
            name: 'between'
            body: 'lower_key, upper_key [, key_attribute]'
            examples: [
              description: "Find all users with primary keys between 10 and 20, inclusive. The primary key is expected to be in an attribute 'id'"
              code: "r.table('marvel').between(10, 20).run()"
            ,
              description: "Find all users with primary keys between 10 and 20, inclusive. The primary key is expected to be in an attribute 'user_id'"
              code: "r.table('marvel').between(10, 20, 'user_id').run()"
            ]
          py:
            name: 'between'
            body: 'lower_key, upper_key [, key_attribute]'
            examples: [
              description: "Find all users with primary keys between 10 and 20, inclusive. The primary key is expected to be in an attribute 'id'"
              code: "r.table('marvel').between(10, 20).run()"
            ,
              description: "Find all users with primary keys between 10 and 20, inclusive. The primary key is expected to be in an attribute 'user_id'"
              code: "r.table('marvel').between(10, 20, 'user_id').run()"
            ]
          ry:
            name: 'between'
            body: 'lower_key, upper_key [, key_attribute]'
            examples: [
              description: "Find all users with primary keys between 10 and 20, inclusive. The primary key is expected to be in an attribute 'id'"
              code: "r.table('marvel').between(10, 20).run"
            ,
              description: "Find all users with primary keys between 10 and 20, inclusive. The primary key is expected to be in an attribute 'user_id'"
              code: "r.table('marvel').between(10, 20, 'user_id').run"
            ]
      # Filter documents out
      ###
      filter:
        description: "Get all documents that match a condition. A condition can be expressed as a ReQL expression, or as a JSON document."
        parent: 'stream'
        returns: 'stream'
        langs:
          js:
            name: 'filter'
            body: 'condition'
            examples: [
              description: "Find all users aged between 10 and 20"
              code: "r.table('marvel').between(10, 20).run()"
            ,
              description: "Find all users with primary keys between 10 and 20, inclusive. The primary key is expected to be in an attribute 'user_id'"
              code: "r.table('marvel').between(10, 20, 'user_id').run()"
            ]
       ###
    ]
]

# table create primary key, dc id
# todo objects: pick, unpick, extend, (attr)
