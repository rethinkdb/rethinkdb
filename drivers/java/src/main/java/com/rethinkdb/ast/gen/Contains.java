// Autogenerated by metajava.py.
// Do not edit this file directly.
// The template for this file is located at:
// ../../../../../../../../templates/AstSubclass.java
package com.rethinkdb.ast.gen;

import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.proto.TermType;


public class Contains extends ReqlQuery {


    public Contains(java.lang.Object arg) {
        this(new Arguments(arg), null);
    }
    public Contains(Arguments args, OptArgs optargs) {
        this(null, args, optargs);
    }
    public Contains(ReqlAst prev, Arguments args, OptArgs optargs) {
        this(prev, TermType.CONTAINS, args, optargs);
    }
    protected Contains(ReqlAst previous, TermType termType, Arguments args, OptArgs optargs){
        super(previous, termType, args, optargs);
    }


    /* Static factories */
    public static Contains fromArgs(java.lang.Object... args){
        return new Contains(new Arguments(args), null);
    }


}