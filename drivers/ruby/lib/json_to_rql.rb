require 'oj'

module RethinkDB
  # Convert JSON to RQL protocol buffers
  class JSONToRQL < Oj::Saj
    def initialize
      @parents = []
      @current = nil
    end

    # Return resulting Term object
    def result
      @current
    end

    # Add table to current container
    def hash_start(key)
      add_container(Term::TermType::MAKE_OBJ, key)
    end

    # Leave current container
    def hash_end(key)
      leave_container
    end

    # Add a new array to current container
    def array_start key
      add_container(Term::TermType::MAKE_ARRAY, key)
    end

    # Leave current container
    def array_end(key)
      leave_container
    end

    # Add a new value to an array or key/value pair to table
    def add_value val, key
      if val.is_a? Term
        val = val
      else
        val = RethinkDB::Shim.native_to_datum_term(val)
      end

      if key # Table
        pair = Term::AssocPair.new
        pair.key = key.to_s  # Probably already string
        pair.val = val

        @current.optargs << pair
      else # Array
        @current.args << val
      end
    end

    def add_container(container_type, key = nil)
      @parents << @current unless @current.nil?

      container = Term.new
      container.type = container_type

      if @current
        add_value(container, key)
      end

      @current = container
    end

    def leave_container
      @current = @parents.pop unless @parents.empty?
    end
  end

  class RQL
    # Convert JSON string or IO object to RQL expression
    def json(input)
      converter = JSONToRQL.new
      Oj.saj_parse(converter, input)
      RQL.new(converter.result)
    end
  end
end
