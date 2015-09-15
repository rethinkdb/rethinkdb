package com.rethinkdb.gen.ast;

import com.rethinkdb.model.ReqlLambda;
import com.rethinkdb.ast.ReqlAst;

public interface ReqlFunction${arity} extends ReqlLambda {
    ReqlAst apply(${", ".join("ReqlExpr arg%s" % (i,) for i in range(1,arity+1))});
}
