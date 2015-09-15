# Python Driver

For more information about this driver see the [Python API](http://www.rethinkdb.com/api/python/) page.

# Contributing

## Installing
If you would like to start helping develop the python driver you need to do
the following:

```bash
git clone https://github.com/rethinkdb/rethinkdb.git
cd rethinkdb
./configure --allow-fetch
make
cd drivers/python
make
pip install -e .
```

Since the python driver requires you to run the `make` command for the whole
server it will take awhile, if you have a lot of cores you can speed it up by
running:

```bash
make -j 8
```

`8` meaning use 8 cores to compile this.

After you have everything installed you can test it by running this command:

```bash
python -c 'import rethinkdb'
```

## Running the tests

To run the tests you can use tox:

```bash
tox -e py27
```
