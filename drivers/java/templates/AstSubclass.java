<%page args="term_name, classname, superclass, all_terms" />
package com.rethinkdb.gen.ast;

import com.rethinkdb.gen.proto.TermType;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.ast.ReqlAst;

<%block name="add_imports" />

public class ${classname} extends ${superclass} {
<%block name="member_vars" />
<%block name="constructors">
% if term_name is not None:
    public ${classname}(java.lang.Object arg) {
        this(new Arguments(arg), null);
    }
    public ${classname}(Arguments args, OptArgs optargs) {
        this(null, args, optargs);
    }
    public ${classname}(ReqlAst prev, Arguments args, OptArgs optargs) {
        this(prev, TermType.${term_name}, args, optargs);
    }
% endif
    protected ${classname}(ReqlAst previous, TermType termType, Arguments args, OptArgs optargs){
        super(previous, termType, args, optargs);
    }
</%block>
<%block name="static_factories">
    /* Static factories */
% if term_type is not None:
    public static ${classname} fromArgs(Object... args){
        return new ${classname}(new Arguments(args), null);
    }
%endif
</%block>
<%block name="special_methods" />
% for term, info in all_terms.iteritems():
    % if classname in info.get('include_in'):
    % for signatures in info.get('signatures', []):
    public ${info['classname']} ${info['methodname']}() {
        return new ${info['classname']}(this, new Arguments(fields), new OptArgs());
    }
    % endfor
    % endif
% endfor
}
