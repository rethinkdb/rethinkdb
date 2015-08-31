<%inherit file="../AstSubclass.java" />

<%block name="special_methods">
    public static ${classname} fromString(String iso) {
        return new ${classname}(new Arguments(iso), null);
    }
</%block>
