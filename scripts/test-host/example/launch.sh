# Copyright 2010-2012 RethinkDB, all rights reserved.
if [ -z $TEST_HOST ]
then
    echo "You must define TEST_HOST" 2>&1
    exit 1
fi
tar --create -z --file=- renderer test.py | curl -X POST http://$TEST_HOST/spawn/ -F tarball=@- -F command='python test.py' -F title=a_stupid_test $*
