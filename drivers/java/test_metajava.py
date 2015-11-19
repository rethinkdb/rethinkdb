import metajava as MJ

import pytest


@pytest.mark.parametrize(("sig", "expected"), [
    ([],
     [[]]),
    (['T_DB'],
     [['Db']]),
    (['T_DB', 'T_EXPR'],
     [['Db', 'Object']]),
    (['T_EXPR', '*'],
     [['Object...']]),
    (['T_EXPR', 'T_EXPR'],
     [['Object', 'Object']]),
    (['T_EXPR', 'T_EXPR', '*'],
     [['Object', 'Object...']]),
    (['T_EXPR', 'T_EXPR', 'T_EXPR'],
     [['Object', 'Object', 'Object']]),
    (['T_EXPR', 'T_EXPR', 'T_EXPR', '*'],
     [['Object', 'Object', 'Object...']]),
    (['T_EXPR', 'T_EXPR', 'T_EXPR', 'T_EXPR'],
     [['Object', 'Object', 'Object', 'Object']]),
    (['T_EXPR', 'T_EXPR', 'T_EXPR', 'T_EXPR', '*'],
     [['Object', 'Object', 'Object', 'Object...']]),
    (['T_EXPR', 'T_EXPR', 'T_EXPR',
      'T_EXPR', 'T_EXPR', 'T_EXPR', 'T_EXPR'],
     [['Object', 'Object', 'Object',
       'Object', 'Object', 'Object', 'Object']]),
    (['T_EXPR', 'T_EXPR', 'T_FUNC2'],
     [['Object', 'Object', 'ReqlFunction2']]),
    (['T_EXPR', 'T_FUNC1'],
     [['Object', 'ReqlFunction1']]),
    (['T_EXPR', 'T_FUNC1', 'T_EXPR'],
     [['Object', 'ReqlFunction1', 'Object']]),
    (['T_EXPR', 'T_FUNC2'],
     [['Object', 'ReqlFunction2']]),
    (['T_TABLE'],
     [['Table']]),
    (['T_TABLE', 'T_EXPR'],
     [['Table', 'Object']]),
    (['T_TABLE', 'T_EXPR', '*'],
     [['Table', 'Object...']]),
    (['T_TABLE', 'T_EXPR', 'T_EXPR'],
     [['Table', 'Object', 'Object']]),
    (['T_TABLE', 'T_EXPR', 'T_FUNC1'],
     [['Table', 'Object', 'ReqlFunction1']]),
    (['T_EXPR', ['T_EXPR', 'T_FUNC1'], '*'],
     [['Object'],
      ['Object', 'Object'],
      ['Object', 'ReqlFunction1'],
      ['Object', 'Object', 'Object'],
      ['Object', 'Object', 'ReqlFunction1'],
      ['Object', 'ReqlFunction1', 'Object'],
      ['Object', 'ReqlFunction1', 'ReqlFunction1']]),
    (['T_EXPR', 'T_EXPR', '*', 'T_FUNCX'],
     [['Object', 'ReqlFunction1'],
      ['Object', 'Object', 'ReqlFunction2'],
      ['Object', 'Object', 'Object', 'ReqlFunction3'],
      ['Object', 'Object', 'Object', 'Object', 'ReqlFunction4']]),
    (['T_EXPR', 'T_EXPR', 'T_EXPR', '*', 'T_FUNCX'],
     [['Object', 'Object', 'ReqlFunction2'],
      ['Object', 'Object', 'Object', 'ReqlFunction3'],
      ['Object', 'Object', 'Object', 'Object', 'ReqlFunction4'],
      ['Object', 'Object', 'Object', 'Object', 'Object', 'ReqlFunction5']]),
])
def test_reify_signatures(sig, expected):
    assert MJ.java_term_info.reify_signature(sig) == expected


def test_elaborate_signature():
    sig = ["Object", "Object", "Object...", "ReqlFunction2", "ReqlFunction3"]
    expected = {
        "args": [
            {"type": "Object", "var": "expr"},
            {"type": "Object", "var": "exprA"},
            {"type": "Object...", "var": "exprs"},
            {"type": "ReqlFunction2", "var": "func2"},
            {"type": "ReqlFunction3", "var": "func3"},
        ],
        "first_arg": "ReqlExpr"
    }

    assert MJ.java_term_info.elaborate_signature(sig) == expected
