<%inherit file="../AstSubclass.java" />

<%block name="member_vars">
    public final java.lang.Object datum;</%block>
<%block name="constructors">
    public Datum(java.lang.Object arg) {
        super(TermType.DATUM, null, null);
        datum = arg;
    }
</%block>

<%block name="static_factories"/>

<%block name="special_methods">
    @Override
    protected Object build() {
        // Overridden because Datums are leaf-nodes and therefore
        // don't contain lower ReqlAst objects.
        return datum;
    }
</%block>
