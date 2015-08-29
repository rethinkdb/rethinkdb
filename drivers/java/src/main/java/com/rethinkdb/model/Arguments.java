package com.rethinkdb.model;


import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.ast.Util;

import java.util.*;
import java.util.stream.Collectors;

public class Arguments extends ArrayList<ReqlAst> {

    public Arguments() {}
    public Arguments(Object arg) {
        coerceAndAdd(arg);
    }
    public Arguments(ReqlAst arg) {
        add(arg);
    }

    public Arguments(Object[] args) {
        coerceAndAdd(args);
    }

    public Arguments(List<Object> args) {
        addAll(Collections.singletonList(args).stream()
                .map(Util::toReqlAst)
                .collect(Collectors.toList()));
    }

    public static Arguments make(Object... args){
        return new Arguments(args);
    }

    public void coerceAndAdd(Object obj) {
        add(Util.toReqlAst(obj));
    }

    public void coerceAndAddAll(Object[] args) {
        addAll(Collections.singletonList(args).stream()
                .map(Util::toReqlAst)
                .collect(Collectors.toList()));
    }
}
