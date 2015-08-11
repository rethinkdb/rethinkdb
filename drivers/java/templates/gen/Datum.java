<%inherit file="../AstSubclass.java" />

<%block name="add_imports">
import org.json.simple.JSONArray;
</%block>

<%block name="member_vars">
    protected final java.lang.Object datum;
</%block>
<%block name="constructors">
    public Datum(java.lang.Object arg) {
        super(null, TermType.DATUM, null, null);
        datum = arg;
    }
</%block>

<%block name="static_factories"/>

<%block name="special_methods">
    @Override
    protected JSONArray build() {
        // Overridden because Datums are leaf-nodes and therefore
        // don't contain lower ReqlAst objects.
        JSONArray list = new JSONArray();
        list.add(datum);
        return list;
    }
</%block>
