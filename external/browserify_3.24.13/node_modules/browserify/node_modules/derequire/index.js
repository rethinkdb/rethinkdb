'use strict';
var estraverse = require('estraverse');
var esprima = require('esprima-six');
var esrefactor = require('esrefactor');
function testParse (code) {
    try{
         return esprima.parse(code,{range:true});
    }catch(e){}
}
function rename(code, tokenTo, tokenFrom) {
    tokenTo = tokenTo || '_dereq_';
    tokenFrom = tokenFrom || 'require';
    if(tokenTo.length !== tokenFrom.length){
        throw new Error('bad stuff will happen if you try to change tokens of different length');
    }
    var inCode = '!function(){'+code+'\n;}';
    var ast = testParse(inCode);
    if(!ast){
        return code;
    }
    var ctx = new esrefactor.Context(inCode);;
   
    estraverse.traverse(ast,{
        enter:function(node, parent) {
            var test = 
            parent &&
            (parent.type === 'FunctionDeclaration' || parent.type === 'FunctionExpression') &&
            node.name === tokenFrom &&
            node.type === 'Identifier';
            if(test){
                ctx._code = ctx.rename(ctx.identify(node.range[0]), tokenTo);
            }
        } 
    });
    return ctx._code.slice(12, -3);
}



module.exports = rename;
