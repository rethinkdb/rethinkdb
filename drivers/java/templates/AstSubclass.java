<%page args="term_name, classname, superclass, all_terms" />
package com.rethinkdb.gen.ast;

import com.rethinkdb.gen.proto.TermType;
import com.rethinkdb.gen.exc.ReqlDriverError;
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
        super(TermType.${term_name}, args, optargs);
    }
    %else:
    protected ${classname}(TermType termType, Arguments args, OptArgs optargs){
        super(termType, args, optargs);
    }
    %endif
</%block>\
<%block name="static_factories"></%block>\
<%block name="optArgs">\
% if optargs:
% for type in ["Object", "ReqlFunction0", "ReqlFunction1", "ReqlFunction2", "ReqlFunction3", "ReqlFunction4"]:
public ${classname} optArg(String optname, ${type} value) {
    OptArgs newOptargs = OptArgs.fromMap(optargs).with(optname, value);
    return new ${classname}(args, newOptargs);
}
% endfor
% endif
</%block>
<%block name="special_methods" />\
% for term, info in all_terms.items():
  % if classname in info.get('include_in'):
    % for methodname in info['methodnames']:
      % for signature in info['signatures']:
        % if signature['first_arg'] == classname:
          % if signature['args'][0]['type'].endswith('...'):
<% remainingArgs = signature['args'] %>\
          % else:
<% remainingArgs = signature['args'][1:] %>\
          % endif
    public ${info['classname']} ${methodname}(${
            ', '.join("%s %s" % (arg['type'], arg['var'])
                      for arg in remainingArgs)}) {
        Arguments arguments = new Arguments(this);
          %for arg in remainingArgs:
            %if arg['type'].endswith('...'):
        arguments.coerceAndAddAll(${arg['var']});
            %else:
        arguments.coerceAndAdd(${arg['var']});
            %endif
          %endfor
        return new ${info['classname']}(arguments);
    }
        % endif
      % endfor
    % endfor
  % endif
% endfor
}
