<%inherit file="../AstSubclass.java" />

<%block name="add_imports">
import com.rethinkdb.model.RqlFunction2;
import com.rethinkdb.model.RqlFunction;
import com.rethinkdb.model.RqlLambda;
import com.rethinkdb.ast.Util;
</%block>

<%block name="constructors">
    public Func(RqlFunction function) {
        this(Arguments.make(
            new MakeArray(new Arguments(1), null),
                Util.toRqlAst(function.apply(new Var(1)))));
    }

    public Func(RqlFunction2 function) {
        this(Arguments.make(
            new MakeArray(Arguments.make(1, 2)),
                Util.toRqlAst(function.apply(new Var(1), new Var(1)))));
    }

    protected Func(Arguments args) {
        super(null, TermType.FUNC, args, null);
    }
</%block>

<%block name="static_factories" />
