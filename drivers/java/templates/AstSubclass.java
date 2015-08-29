<%page args="term_name, classname, superclass, all_terms" />
package com.rethinkdb.gen.ast;

import com.rethinkdb.gen.proto.TermType;
import com.rethinkdb.gen.model.TopLevel;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.ast.ReqlAst;

<%block name="add_imports" />

public class ${classname} extends ${superclass} {
<%block name="member_vars" />
<%block name="constructors">
    %if term_name is not None:
    public ${classname}(Object arg) {
        this(new Arguments(arg), null);
    }
    public ${classname}(Arguments args){
        this(args, null);
    }
    public ${classname}(Arguments args, OptArgs optargs) {
        this(TermType.${term_name}, args, optargs);
    }
    %endif
    protected ${classname}(TermType termType, Arguments args, OptArgs optargs){
        super(termType, args, optargs);
    }
</%block>\
<%block name="static_factories"></%block>\
<%block name="special_methods" />\
% for term, info in all_terms.iteritems():
  % if classname in info.get('include_in'):
    % for signature in info.get('signatures', []):
      % if signature['first_arg'] == classname:
    public ${info['classname']} ${info['methodname']}(${
            ', '.join("%s %s" % (arg['type'], arg['var'])
                      for arg in signature['args'][1:])}) {
        Arguments arguments = new Arguments(this);
        %for arg in signature['args'][1:]:
          %if arg['type'] == 'Object...':
        arguments.coerceAndAddAll(${arg['var']});
          %else:
        arguments.coerceAndAdd(${arg['var']});
          %endif
        %endfor
        return new ${info['classname']}(arguments);
    }
      % endif
    % endfor
  % endif
% endfor
}
