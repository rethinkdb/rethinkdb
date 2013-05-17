# Load the RethinkDB module.
require 'rethinkdb'

# Make the RethinkDB API accessible by the name 'r'.
include RethinkDB::Shortcuts

# Open a connection.
conn = r.connect('localhost', 28015, 'test')

# Create a table and insert three rows.
r.table_create('tristate_area').run(conn)
r.table('tristate_area').insert([{:name => "Pennsylvania", :code => 'PA'},
                                 {:name => 'New Jersey', :code => 'NJ'},
                                 {:name => 'Delaware', :code => 'DE'}]).run(conn)

# Make a cursor containing every row of the table and destructively
# iterate through its rows.
cursor = r.table('tristate_area').run(conn)
cursor.each { |x|
  puts x
}

# Cleanup.  (Exception safety is not a part of this example!)
r.table_drop('tristate_area').run(conn)
conn.close()
