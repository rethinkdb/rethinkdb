module RethinkDB
  # A database stored on the cluster.  Usually created with the <b>+r+</b>
  # shortcut, like:
  #   r.db('Welcome-db')
  class Database
    @@default_datacenter='Welcome-dc'
    # Refer to the database named <b>+name+</b>.  Usually you would
    # use the <b>+r+</b> shortcut instead:
    #   r.db(name)
    def initialize(name); @db_name = name; end

    # Access the table <b>+name+</b> in this database.  May also use the table
    # name as a member function for convenience.  The following are equivalent:
    #   r.db('Welcome-db').table('tbl')
    #   r.db('Welcome-db').tbl
    def table(name); Table.new(@db_name, name); end

    # Create a new table in this database.  You should specify the
    # datacenter it should reside in and the primary key as well
    # (almost always "id").  When run, either returns <b>+nil+</b> or
    # throws on error.
    def create_table(name, datacenter=@@default_datacenter,  primary_key="id")
      Meta_Query.new [:create_table, datacenter, [@db_name, name], primary_key]
    end

    # Drop the table <b>+name+</b> from this database.  When run,
    # either returns <b>+nil+</b> or throws on error.
    def drop_table(name)
      Meta_Query.new [:drop_table, @db_name, name]
    end

    # List all the tables in this database.  When run, either returns
    # <b>+nil+</b> or throws on error.
    def list_tables
      Meta_Query.new [:list_tables, @db_name]
    end

    # The following are equivalent:
    #   r.db('a').table('b')
    #   r.db('a').b
    def method_missing(m, *a, &b)
      self.table(m.to_s, *a, &b)
    end
  end

  # A table in a particular RethinkDB database.  If you call a
  # function from Sequence on it, it will be treated as a
  # Stream_Expression reading from the table.
  class Table
    # A table named <b>+name+</b> residing in database <b>+db_name+</b>.

    def initialize(db_name, name);
      @db_name = db_name;
      @table_name = name;
      @body = [:table, @db_name, @table_name] # when used as stream
    end

    # Insert one or more rows into the table.  If you try to insert a
    # row with a primary key already in the table, you will get abck
    # an error.  For example, if you have a table <b>+table+</b>:
    #   table.insert({:id => 1}, {:id => 1})
    # Will return something like:
    #   {'inserted' => 1, 'errors' => 1, 'first_error' => ...}
    # You may also provide a stream.  So to make a copy of a table, you can do:
    #   r.create_db('new_db').run
    #   r.db('new_db').create_table('new_table').run
    #   r.db('new_db').new_table.insert(table).run
    def insert(rows)
      rows = [rows] if rows.class != Array
      Write_Query.new [:insert, [@db_name, @table_name], rows.map{|x| S.r(x)}, false]
    end

    def upsert(rows) # :nodoc:
      rows = [rows] if rows.class != Array
      Write_Query.new [:insert, [@db_name, @table_name], rows.map{|x| S.r(x)}, true]
    end
    # Get the row of the invoking table with key <b>+key+</b>.  You may also
    # optionally specify the name of the attribute to use as your key
    # (<b>+keyname+</b>), but note that your table must be indexed by that
    # attribute.  For example, if we have a table <b>+table+</b>:
    #   table.get(0)
    def get(key, keyname=:id)
      Single_Row_Selection.new [:getbykey, [@db_name, @table_name], keyname, S.r(key)]
    end

    def method_missing(m, *args, &block) # :nodoc:
      Multi_Row_Selection.new(@body).send(m, *args, &block);
    end

    def to_mrs # :nodoc:
      Multi_Row_Selection.new(@body)
    end
  end
end
