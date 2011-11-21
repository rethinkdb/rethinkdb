if [ -z $TEST_HOST ]
then
    echo "You must define TEST_HOST" 2>&1
    exit 1
fi
if [ -z $1 ]
then
    echo "You must give an email address to send results to" 2>&1
    exit 1
fi
EMAILEE=$1; shift
tar --create -z --file=- full_test_driver.py remotely.py simple_linear_db.py renderer static | \
    curl -X POST http://$TEST_HOST/spawn/ \
        -F tarball=@- \
        -F command="SLURM_CONF=/home/tim/slurm/slurm.conf ./full_test_driver.py $* >output.txt 2>&1" \
        -F title="Nightly test" \
        -F emailee=$EMAILEE
