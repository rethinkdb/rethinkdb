package com.rethinkdb.gen.ast;

import com.rethinkdb.model.ReqlLambda;
import com.rethinkdb.ast.ReqlAst;

public interface ReqlFunction${arity} extends ReqlLambda {
    ReqlAst apply(${", ".join("ReqlAst arg%s" % (i,) for i in xrange(1,arity+1))});
}
