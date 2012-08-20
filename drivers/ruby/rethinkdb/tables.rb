module RethinkDB
  # A database stored on the cluster.  Usually created with the <b>+r+</b>
  # shortcut, like:
  #   r.db('Welcome-db')
  class Database
    @@default_datacenter='Welcome-dc'
    # Create a new database named <b>+name+</b>.  Usually use the <b>+r+</b>
    # shortcut instead:
    #   r.db(name)
    def initialize(name); @db_name = name; end

    # Access the table <b>+name+</b> in the database.  May also use the table
    # name as a member function for convenience.  The following are equivalent:
    #   r.db('Welcome-db').table('tbl')
    #   r.db('Welcome-db').tbl
    def table(name); Table.new(@db_name, name); end

    # Create a new table.  You should specify the datacenter it should reside in
    # and the primary key as well (almost always "id").  When run, either
    # returns <b>+nil+</b> or throws on error.
    def create_table(name, datacenter=@@default_datacenter,  primary_key="id")
      Meta_Query.new [:create_table, datacenter, [@db_name, name], primary_key]
    end

    # Drop a table from the database.  When run, either returns <b>+nil+</b> or
    # throws on error.
    def drop_table(name)
      Meta_Query.new [:drop_table, @db_name, name]
    end

    # List all the tables in the database when run.
    def list_tables
      Meta_Query.new [:list_tables, @db_name]
    end
  end

  # A table in a particular RethinkDB database.  Also acts as a stream query.
  class Table < Stream_Query
    # A table named <b>+name+</b> residing in database <b>+db_name+</b>.
    def initialize(db_name, name);
      @db_name = db_name;
      @table_name = name;
      @body = [:table, @db_name, @table_name] # when used as stream
    end

    # Insert one or more rows into the table.  Rows with duplicate primary key
    # (usually :id) are ignored, with no guarantees about which row 'wins'.  May
    # also provide only one row to insert.  For example, if we have a table
    # <b>+table+</b>, the following are equivalent:
    #   table.insert({:id => 1, :name => 'Bob'})
    #   table.insert([{:id => 1, :name => 'Bob'}])
    #   table.insert([{:id => 1, :name => 'Bob'}, {:id => 1, :name => 'Bob'}])
    def insert(rows)
      rows = [rows] if rows.class != Array
      Write_Query.new [:insert, [@db_name, @table_name], rows.map{|x| RQL.expr x}]
    end

    # Insert a stream into the table.  Often used to insert one table into
    # another.  For example:
    #   table.insertstream(table2.filter{|row| row[:id] < 5})
    # will insert every row in <b>+table2+</b> with id less than 5 into
    # <b>+table+</b>.
    def insertstream(stream)
      Write_Query.new [:insertstream, [@db_name, @table_name], stream]
    end

    # Get the row of the invoking table with key <b>+key+</b>.  You may also
    # optionally specify the name of the attribute to use as your key
    # (<b>+keyname+</b>), but note that your table must be indexed by that
    # attribute.  For example, if we have a table <b>+table+</b>:
    #   table.get(0)
    def get(key, keyname=:id)
      Point_Read_Query.new [:getbykey, [@db_name, @table_name], keyname, RQL.expr(key)]
    end
  end
end
