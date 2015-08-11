package com.rethinkdb.ast.gen;

import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.proto.TermType;
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
    public ${classname}(ReqlAst prev, Arguments args, OptArgs optargs) {
        this(prev, TermType.${term_type}, args, optargs);
    }
% endif
    protected ${classname}(ReqlAst previous, TermType termType, Arguments args, OptArgs optargs){
        super(previous, termType, args, optargs);
    }
</%block>
<%block name="static_factories">
    /* Static factories */
% if term_type is not None:
    public static ${classname} fromArgs(java.lang.Object... args){
        return new ${classname}(new Arguments(args), null);
    }
%endif
</%block>
<%block name="special_methods" />
% for term, info in meta.iteritems():
    % if include_in in info.get('include_in', ['query']):
    public ${camel(term)} ${info.get('alias', dromedary(term))}(java.lang.Object... fields) {
     %if include_in == "top":
        return new ${camel(term)}(null, new Arguments(fields), new OptArgs());
     %else:
        return new ${camel(term)}(this, new Arguments(fields), new OptArgs());
     %endif
    }

    % endif
% endfor
}
