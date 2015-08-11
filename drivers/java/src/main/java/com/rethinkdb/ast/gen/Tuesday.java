// Autogenerated by metajava.py.
// Do not edit this file directly.
// The template for this file is located at:
// ../../../../../../../../templates/AstSubclass.java
package com.rethinkdb.ast.gen;

import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.proto.TermType;


public class Tuesday extends ReqlQuery {


    public Tuesday(java.lang.Object arg) {
        this(new Arguments(arg), null);
    }
    public Tuesday(Arguments args, OptArgs optargs) {
        this(null, args, optargs);
    }
    public Tuesday(ReqlAst prev, Arguments args, OptArgs optargs) {
        this(prev, TermType.TUESDAY, args, optargs);
    }
    protected Tuesday(ReqlAst previous, TermType termType, Arguments args, OptArgs optargs){
        super(previous, termType, args, optargs);
    }


    /* Static factories */
    public static Tuesday fromArgs(java.lang.Object... args){
        return new Tuesday(new Arguments(args), null);
    }


}