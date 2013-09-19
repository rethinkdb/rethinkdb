import json
import sys
import re

docs_file = "../../build/docs/reql_docs.json"

docs = json.load(open(docs_file))

parents = {
	None: '',
	'r': 'rethinkdb.',
	'connection': 'rethinkdb.net.Connection.',
	'cursor': 'rethinkdb.net.Cursor.',
	'db': 'rethinkdb.ast.DB.',
	'table': 'rethinkdb.ast.Table.',
}

tags = {
	'and': '__and__.__func__',
	'or': '__or__.__func__',
	'[]': '__getitem__.__func__',
        '+': '__add__.__func__',
        '-': '__sub__.__func__',
        '*': '__mul__.__func__',
        '/': '__div__.__func__',
        '%': '__mod__.__func__',
        '&': '__and__.__func__',
        '|': '__or__.__func__',
        '==': '__eq__.__func__',
        '!=': '__neq__.__func__',
        '<': '__lt__.__func__',
        '>': '__gt__.__func__',
        '<=': '__le__.__func__',
        '>=': '__ge__.__func__',
	'~': '__not__.__func__',
	'r': 'rethinkdb',
	'connect': 'connect',
	'next': None,
	'close-cursor': None,
	'has_next': None,
	'each': None,
	'to_array': None,
	'db_create': 'db_create',
	'db_drop': 'db_drop',
	'db_list': 'db_list',
	'db': 'db',
	'inner': None,
	'outer': None,
	'groupedmapreduce': 'grouped_map_reduce.__func__',
	'count': None,
	'sum': 'sum',
	'avg': 'avg',
	'row': 'row',
	'do': 'do',
	'branch': 'branch',
	'foreach': None,
	'error': 'error',
	'expr': 'expr',
	'js': 'js',
}

print 'import rethinkdb'

for section in docs['sections']:
	for command in section['commands']:
		doc = command['description']
		for example in command['langs']['py']['examples']:
                        doc = doc + '\n\n' + example['description'] + \
                              '\n>>> ' + example['code']
		print '#', command['io'][0][0], command['tag']
		tag = command['langs']['py'].get('name', command['tag'])
		tag = tags.get(tag, tag + '.__func__')
		if tag:
			print parents.get(command['io'][0][0],
                                          'rethinkdb.ast.RqlQuery.') + \
                              tag + '.__doc__ = ' + repr(doc)
