package com.rethinkdb.model;

import com.rethinkdb.ast.RqlAst;

public interface RqlFunction2 extends RqlLambda {
    RqlAst apply(RqlAst row1, RqlAst row2);
}
