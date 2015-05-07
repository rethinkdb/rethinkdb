<%inherit file="../AstSubclass.java" />
<%block name="constructors">
    public MakeObj(java.util.Map<String, Object> fields){
        super(null, TermType.MAKE_OBJ, new Arguments(), fields);
    }
</%block>
