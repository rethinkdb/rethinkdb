package com.rethinkdb.ast.helper;


import com.rethinkdb.ast.RqlAst;
import com.rethinkdb.ast.Util;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

public class Arguments extends ArrayList<RqlAst> {

    public Arguments() {}
    public Arguments(Object arg1) {
        add(Util.toRqlAst(arg1));
    }

    public Arguments(Object[] args) {
        this(Arrays.asList(args));
    }

    public Arguments(List<Object> args) {
        addAll(args.stream()
               .map(Util::toRqlAst)
               .collect(Collectors.toList()));
    }

    public static Arguments make(Object... args){
        return new Arguments(args);
    }
}
