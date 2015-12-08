<%inherit file="../AstSubclass.java" />

<%block name="special_methods">
    @Override
    protected Object build()
    {
        /*
          This object should be constructed with arguments first, and the
          function itself as the last parameter.  This makes it easier for
          the places where this object is constructed.  The actual wire
          format is function first, arguments last, so we flip them around
          when building the AST.
        */
        ReqlAst func = args.remove(args.size()-1);
        args.add(0, func);
        return super.build();
    }
</%block>
