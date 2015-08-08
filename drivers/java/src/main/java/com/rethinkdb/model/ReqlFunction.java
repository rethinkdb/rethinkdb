package com.rethinkdb.model;

import com.rethinkdb.ast.ReqlAst;

public interface ReqlFunction extends ReqlLambda {
    ReqlAst apply(ReqlAst row);
}
