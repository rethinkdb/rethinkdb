package com.rethinkdb.model;

import com.rethinkdb.ast.helper.OptArgs;

import java.util.*;


public class GlobalOptions {

    % for arg, spec in global_optargs:
    private ${reql_to_java_type(spec['type'])} _${dromedary(arg)};
    % endfor

    public GlobalOptions() {
        % for arg, spec in global_optargs:
        _${dromedary(arg)} = ${java_repr(spec['default'])};
        % endfor
    }

    public OptArgs toOptArgs() {
        return (new OptArgs())
        %for arg, spec in global_optargs:
            .with("${arg}", _${dromedary(arg)})
        %endfor
        ;
    }

    % for arg, spec in global_optargs:
    public GlobalOptions ${dromedary(arg)}(${reql_to_java_type(spec['type'])} ${dromedary(arg)}) {
        _${dromedary(arg)} = ${dromedary(arg)};
        return this;
    }

    % endfor

}
