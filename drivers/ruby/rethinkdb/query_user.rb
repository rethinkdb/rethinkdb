module RethinkDB
  # An RQL query that can be sent to the RethinkDB cluster.  This class contains
  # only functions that work for any type of query.  Queries are either
  # constructed by methods in the RQL module, or by invoking the instance
  # methods of a query on other queries.
  class RQL_Query
    # Run the invoking query using the most recently opened connection.  See
    # Connection#run for more details.
    def run
      Connection.last.send(:run, self)
    end
  end
end
