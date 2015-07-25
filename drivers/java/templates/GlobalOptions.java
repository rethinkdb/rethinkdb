package com.rethinkdb.model;

import com.rethinkdb.ast.helper.OptArgs;

import java.util.*;


public class GlobalOptions {

    % for arg, spec in global_optargs:
    private Optional<${reql_to_java_type(spec['type'])}> _${dromedary(arg)} = Optional.empty();
    % endfor

    public OptArgs toOptArgs() {
        OptArgs ret = new OptArgs();

        %for arg, spec in global_optargs:
        _${dromedary(arg)}.ifPresent(val ->
            ret.with("${arg}", val));
        %endfor

        return ret;
    }

    % for arg, spec in global_optargs:
    public GlobalOptions ${dromedary(arg)}(${reql_to_java_type(spec['type'])} ${dromedary(arg)}) {
        _${dromedary(arg)} = Optional.of(${dromedary(arg)});
        return this;
    }

    public Optional<${reql_to_java_type(spec['type'])}> ${dromedary(arg)}() {
        return _${dromedary(arg)};
    }

    % endfor

}
