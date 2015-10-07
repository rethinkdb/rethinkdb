<%inherit file="../AstSubclass.java"/>

<%block name="special_methods">
    @Override
    public Object apply(ReqlExpr arg1) {
       // This is only to satisfy the ReqlFunction1 interface. toReqlAst
       // will accept the Javascript term itself, it won't try to use
       // function application on the term.
       throw new ReqlDriverError("Apply should not be invoked");
    }
</%block>
