Documentation
--------------------

### Content

This folder contains two files

* py_docs.json
* reql_docs.json


These two files are generated, __do not manually edit them__.
We didn't want to use a sub module, so we commited them.

### Generate

Generate the two files

```
git clone https://github.com/rethinkdb/docs
cd docs
python _scripts/dataexplorer.py
python _scripts/gen_python.py
cp _scripts/*.json ~/path/to/this/folder/
```
