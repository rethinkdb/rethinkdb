<%page args="all_terms" />
package com.rethinkdb.gen.model;

import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.gen.ast.Error;
import com.rethinkdb.gen.ast.*;
import com.rethinkdb.ast.Util;

public class TopLevel {

    public ReqlExpr expr(Object value){
        return Util.toReqlExpr(value);
    }

%for term in all_terms.values():
  %if "TopLevel" in term["include_in"]:
    %for signature in term['signatures']:
    public ${term['classname']} ${term['methodname']}(${
        ', '.join('%s %s' % (arg['type'], arg['var'])
                  for arg in signature['args'])}){
        Arguments args = new Arguments();
        %for arg in signature['args']:
          %if arg['type'] == 'Object...':
        args.coerceAndAddAll(${arg['var']});
          %else:
        args.coerceAndAdd(${arg['var']});
          %endif
        %endfor
        return new ${term['classname']}(args);
    }
    %endfor
  %endif
%endfor
}
