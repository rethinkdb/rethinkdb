import json
import sys
import re

docs_file = "../../build/release/rethinkdb_web_assets/js/reql_docs.json"

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
	'not': None,
	'r': 'rethinkdb',
	'connect': 'connect',
	'next': None,
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
	'getattr': None,
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
			doc = doc + '\n\n' + example['description'] + '\n>>> ' + example['code']
		print '#', command['parent'], command['tag']
		tag = re.sub('([A-Z])', lambda x: '_' + x.group().lower(), command['tag'])
		tag = tags.get(tag, tag + '.__func__')
		if tag:
			print parents.get(command['parent'], 'rethinkdb.ast.RqlQuery.') + tag + '.__doc__ = ' + repr(doc)
