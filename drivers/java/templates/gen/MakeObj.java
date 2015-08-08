<%inherit file="../AstSubclass.java" />

<%block name="special_methods">
    public static MakeObj fromMap(java.util.Map<String, ReqlAst> map){
        return new MakeObj(OptArgs.fromMap(map));
    }
</%block>
