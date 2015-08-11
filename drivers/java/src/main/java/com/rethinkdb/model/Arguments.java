package com.rethinkdb.model;


import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.ast.Util;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

public class Arguments extends ArrayList<ReqlAst> {

    public Arguments() {}
    public Arguments(Object arg1) {
        add(Util.toReqlAst(arg1));
    }
    public Arguments(ReqlAst arg1) {
        add(arg1);
    }

    public Arguments(Object[] args) {
        this(Arrays.asList(args));
    }

    public Arguments(List<Object> args) {
        addAll(args.stream()
               .map(Util::toReqlAst)
               .collect(Collectors.toList()));
    }

    public static Arguments make(Object... args){
        return new Arguments(args);
    }
}
