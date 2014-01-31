Description
==========

Run basic tests for all terms to look for regression between releases.



Requirements
==========

Python driver with the C++ backend.
Require a release build in ../../build/release/



Run tests
==========

Run tests with:
```
python test.py
```


Add queries
=========
Add queries in `queries.py` with a simple string or an object with two fields (`query` and `tag`)

Note: `tag` must be unique.
