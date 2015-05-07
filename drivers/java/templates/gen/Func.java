<%inherit file="../AstSubclass.java" />

<%block name="add_imports">
import com.rethinkdb.model.RqlFunction2;
import com.rethinkdb.model.RqlFunction;
import com.rethinkdb.model.RqlLambda;
import com.rethinkdb.ast.RqlUtil;
</%block>

<%block name="constructors">
    public Func(RqlLambda function) {
        super(null, TermType.FUNC, null, null);
        if (function instanceof RqlFunction) {
            super.init(
                null,
                Arguments.make(
                    new MakeArray(new Arguments(1), null),
                    RqlUtil.toRqlAst(
                        ((RqlFunction)function).apply(
                            new Var(new Arguments(1), null)))
                ),
                null
            );
        }
        else {
            super.init(
                null,
                Arguments.make(
                    new MakeArray(Arguments.make(1, 2), null),
                    RqlUtil.toRqlAst(
                        ((RqlFunction2)function).apply(
                            new Var(new Arguments(1), null),
                            new Var(new Arguments(2), null)))
                ),
                null
            );
        }
    }
</%block>

<%block name="static_factories" />
