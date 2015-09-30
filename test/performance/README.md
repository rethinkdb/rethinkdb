Description
==========

Run basic tests for all terms to look for regressions between releases.


Requirements
==========

Uses the Python driver
Requires a release build in ../../build/release/


Use
==========

Run tests with:
```
python test.py
```


Make more comparisons
```
python compare <file1> <file2>
```

Where file1 must contain the more recent results.

Ex:
```
python compare  result_14.03.20-15:10:18.txt result_14.03.19-00:35:18.txt
```


Add queries
=========
Add queries in `queries.py` with a simple string or an object with two fields (`query` and `tag`).

Note: `tag` must be unique.
