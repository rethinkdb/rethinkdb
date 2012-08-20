module RethinkDB
  # A meta operation, such as creating a database or dropping a table.  Can be
  # run like a normal query.
  class Meta_Query < RQL_Query; end

  # A write operation, like an insert.
  class Write_Query < RQL_Query; end

  # A stream.
  class Stream_Query < RQL_Query; end

  class Point_Read_Query < RQL_Query; end
end
