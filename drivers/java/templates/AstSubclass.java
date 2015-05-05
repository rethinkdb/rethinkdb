package com.rethinkdb.ast.gen;

import com.rethinkdb.Cursor;
import com.rethinkdb.ast.helper.Arguments;
import com.rethinkdb.ast.helper.OptionalArguments;
import com.rethinkdb.ast.RqlAst;
import com.rethinkdb.proto.TermType;
import java.util.*;

public class ${classname} extends ${superclass} {

% if term_type is not None:
    public ${classname}(RqlAst prev, Arguments args, OptionalArguments optargs) {
        super(prev, TermType.${term_type}, args, optargs);
    }
% endif
    /* Query level terms */
% for term, info in meta.iteritems():
    % if include_in in info.get('include_in', ['query']):
    public ${camel(term)} ${info.get('alias', dromedary(term))}(Object... fields) {
        return new ${camel(term)}(this, new Arguments(fields), new Optargs());
    }

    % endif
% endfor
}
