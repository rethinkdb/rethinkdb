if [ -z $TEST_HOST ]
then
    echo "You must define TEST_HOST" 2>&1
    exit 1
fi
if [ -z $1 ]
then
    echo "You must give a branch to test" 2>&1
    exit 1
fi
if [ -z $2 ]
then
    echo "You must give an email address to send results to" 2>&1
    exit 1
fi
tar --create -z --file=- full_test_driver.py remotely.py renderer templates | \
    curl -X POST http://$TEST_HOST/spawn/ \
        -F tarball=@- \
        -F command="SLURM_CONF=/home/tim/slurm/slurm.conf ./full_test_driver.py $1 >output.txt 2>&1" \
        -F title="Nightly test" \
        -F emailee=$2
