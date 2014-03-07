kdir=${1:-../kernel}
scriptdir=$(dirname $0)
udir=$(dirname $scriptdir)
cat $scriptdir/kernel-files.txt | \
(while read l r; do
    left=$(eval echo $l)
    right=$(eval echo $r)
#    echo $left $right
    diff -up $left $right
done)
