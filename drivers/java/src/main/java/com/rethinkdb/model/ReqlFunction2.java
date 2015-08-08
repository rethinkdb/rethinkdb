package com.rethinkdb.model;

import com.rethinkdb.ast.ReqlAst;

public interface ReqlFunction2 extends ReqlLambda {
    ReqlAst apply(ReqlAst row1, ReqlAst row2);
}
