<%inherit file="../AstSubclass.java" />
<%block name="constructors">\
    public MakeObj(Object arg) {
        this(new Arguments(arg), null);
    }
    public MakeObj(OptArgs opts){
        this(new Arguments(), opts);
    }
    public MakeObj(Arguments args){
        this(args, null);
    }
    public MakeObj(Arguments args, OptArgs optargs) {
        this(TermType.MAKE_OBJ, args, optargs);
    }
    protected MakeObj(TermType termType, Arguments args, OptArgs optargs){
        super(termType, args, optargs);
    }
</%block>
<%block name="special_methods">\
    public static MakeObj fromMap(java.util.Map<String, ReqlAst> map){
        return new MakeObj(OptArgs.fromMap(map));
    }
</%block>
