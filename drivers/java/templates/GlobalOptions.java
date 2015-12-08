package com.rethinkdb.gen.model;

import java.util.*;
import com.rethinkdb.model.*;
import com.rethinkdb.gen.ast.*;

public class GlobalOptions {

    % for option_name, type_ in global_optargs:
    private Optional<${type_}> _${option_name} = Optional.empty();
    % endfor

    public OptArgs toOptArgs() {
        OptArgs ret = new OptArgs();

        %for option_name, type_ in global_optargs:
        _${option_name}.ifPresent(val ->
            ret.with("${option_name}", val));
        %endfor

        return ret;
    }

    % for option_name, type_ in global_optargs:
    public static GlobalOptions with${camel(option_name)}(${type_} ${option_name}) {
        return new GlobalOptions().${option_name}(${option_name});
    }

    public GlobalOptions ${option_name}(${type_} ${option_name}) {
        _${option_name} = Optional.of(${option_name});
        return this;
    }

    public Optional<${type_}> ${option_name}() {
        return _${option_name};
    }

    % endfor

}
