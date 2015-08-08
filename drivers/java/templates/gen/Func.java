<%inherit file="../AstSubclass.java" />

<%block name="add_imports">
import com.rethinkdb.model.ReqlFunction2;
import com.rethinkdb.model.ReqlFunction;
import com.rethinkdb.model.ReqlLambda;
import com.rethinkdb.ast.Util;
</%block>

<%block name="constructors">
    public Func(ReqlFunction function) {
        this(Arguments.make(
            new MakeArray(new Arguments(1), null),
                Util.toReqlAst(function.apply(new Var(1)))));
    }

    public Func(ReqlFunction2 function) {
        this(Arguments.make(
            new MakeArray(Arguments.make(1, 2)),
                Util.toReqlAst(function.apply(new Var(1), new Var(1)))));
    }

    protected Func(Arguments args) {
        super(null, TermType.FUNC, args, null);
    }
</%block>

<%block name="static_factories" />
