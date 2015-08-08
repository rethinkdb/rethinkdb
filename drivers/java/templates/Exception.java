package com.rethinkdb;

import java.util.Optional;
import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.response.Backtrace;

public class ${camel(classname)} extends ${camel(superclass)} {

    Optional<Backtrace> backtrace = Optional.empty();
    Optional<ReqlAst> term = Optional.empty();

    public ${camel(classname)}() {
    }

    public ${camel(classname)}(String message) {
        super(message);
    }

    public ${camel(classname)}(String format, Object... args) {
        super(String.format(format, args));
    }

    public ${camel(classname)}(String message, Throwable cause) {
        super(message, cause);
    }

    public ${camel(classname)}(Throwable cause) {
        super(cause);
    }

    public ${camel(classname)}(String msg, ReqlAst term, Backtrace bt) {
        super(msg);
        this.backtrace = Optional.ofNullable(bt);
        this.term = Optional.ofNullable(term);
    }

    public ${camel(classname)} setBacktrace(Backtrace backtrace) {
        this.backtrace = Optional.ofNullable(backtrace);
        return this;
    }

    public Optional<Backtrace> getBacktrace() {
        return backtrace;
    }

    public ${camel(classname)} setTerm(ReqlAst term) {
        this.term = Optional.ofNullable(term);
        return this;
    }

    public Optional<ReqlAst> getTerm() {
        return this.term;
    }
}
