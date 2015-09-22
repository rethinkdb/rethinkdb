#!/usr/bin/env python2

import ast
import json

def convert_to_dict(node):
    if isinstance(node, list):
        return [convert_to_dict(n) for n in node]
    elif isinstance(node, dict):
        return {k: convert_to_dict(v) for k, v in node.items()}
    elif isinstance(node, ast.AST):
        nodedict = node.__dict__.copy()
        nodedict['<type>'] = node.__class__.__name__
        return convert_to_dict(nodedict)
    elif isinstance(node, (int, float, str, bool, unicode)):
        return node
    else:
        return repr(node)


def get_astjson(expr, mode='eval'):
    asta = ast.parse(expr, mode=mode).body
    if isinstance(asta, list):
        asta = asta[0]
    converted = convert_to_dict(asta)
    return json.dumps(converted, indent=4, sort_keys=True)

def ppast(expr, mode='eval'):
    astjson = get_astjson(expr, mode=mode)
    print(astjson)

if __name__ == "__main__":
    import sys
    ppast(sys.argv[1])