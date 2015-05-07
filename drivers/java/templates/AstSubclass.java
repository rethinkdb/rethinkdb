package com.rethinkdb.ast.gen;

import com.rethinkdb.ast.helper.Arguments;
import com.rethinkdb.ast.helper.OptArgs;
import com.rethinkdb.ast.RqlAst;
import com.rethinkdb.proto.TermType;
import java.util.*;

<%block name="add_imports" />

public class ${classname} extends ${superclass} {
<%block name="member_vars" />
<%block name="constructors">
% if term_type is not None:
    public ${classname}(java.lang.Object arg) {
        this(new Arguments(arg), null);
    }
    public ${classname}(Arguments args, OptArgs optargs) {
        this(null, args, optargs);
    }
    public ${classname}(RqlAst prev, Arguments args, OptArgs optargs) {
        this(prev, TermType.${term_type}, args, optargs);
    }
% endif
    protected ${classname}(RqlAst previous, TermType termType, Arguments args, OptArgs optargs){
        super(previous, termType, args, optargs);
    }
</%block>
<%block name="special_methods" />
% for term, info in meta.iteritems():
    % if include_in in info.get('include_in', ['query']):
    public ${camel(term)} ${info.get('alias', dromedary(term))}(Object... fields) {
        return new ${camel(term)}(this, new Arguments(fields), new OptArgs());
    }

    % endif
% endfor
}
