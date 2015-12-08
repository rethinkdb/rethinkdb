<%inherit file="../AstSubclass.java"/>

<%block name="add_imports">
import com.rethinkdb.net.Converter;
import java.util.Optional;
</%block>

<%block name="member_vars">
    Optional<byte[]> b64Data = Optional.empty();
</%block>

<%block name="constructors">
    public ${classname}(byte[] bytes){
        this(new Arguments());
        b64Data = Optional.of(bytes);
    }
    public ${classname}(Object arg) {
        this(new Arguments(arg), null);
    }
    public ${classname}(Arguments args){
        this(args, null);
    }
    public ${classname}(Arguments args, OptArgs optargs) {
        this(TermType.${term_name}, args, optargs);
    }
    protected ${classname}(TermType termType, Arguments args, OptArgs optargs){
        super(termType, args, optargs);
    }
</%block>

<%block name="special_methods">
    @Override
    public Object build(){
        if(b64Data.isPresent()){
            return Converter.toBinary(b64Data.get());
        }else{
            return super.build();
        }
    }
</%block>
