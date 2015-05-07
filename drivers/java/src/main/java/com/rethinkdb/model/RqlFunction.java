package com.rethinkdb.model;

import com.rethinkdb.ast.RqlAst;

public interface RqlFunction extends RqlLambda {
    RqlAst apply(RqlAst row);
}
