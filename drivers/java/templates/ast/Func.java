<%inherit file="../AstSubclass.java" />

<%block name="add_imports">
import com.rethinkdb.model.ReqlLambda;
import com.rethinkdb.ast.Util;
import java.util.concurrent.atomic.AtomicInteger;
</%block>

<%block name="member_vars">
    private static AtomicInteger varId = new AtomicInteger();
</%block>

<%block name="special_methods">
    private static int nextVarId(){
        return varId.incrementAndGet();
    }
</%block>

<%block name="constructors">
    protected Func(Arguments args){
        super(TermType.FUNC, args, null);
    }
</%block>
<%block name="static_factories">
    public static Func fromLambda(ReqlLambda function) {
        % for i in xrange(1, max_arity+1):
        ${"" if loop.first else "else "}if(function instanceof ReqlFunction${i}){
            ReqlFunction${i} func${i} = (ReqlFunction${i}) function;
            % for j in range(1, i+1):
            int var${j} = nextVarId();
            % endfor
            Arguments varIds = Arguments.make(
                ${", ".join("var%s"%(j,) for j in range(1, j+1))});
            Object appliedFunction = func${i}.apply(
                ${", ".join("new Var(var%s)"%(j,) for m in range(1, j+1))}
            );
            return new Func(Arguments.make(
                  new MakeArray(varIds),
                  Util.toReqlAst(appliedFunction)));
        }
        % endfor
        else {
            throw new ReqlDriverError("Arity of ReqlLambda not recognized!");
        }
    }
</%block>
