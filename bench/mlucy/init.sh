#!/bin/bash
set -e
set -o nounset

export ec2=~/ec2_private
export EC2_PRIVATE_KEY=$ec2/pk.pem
export BENCH_KEY=$ec2/bench.key
export EC2_CERT=$ec2/cert.pem
. $ec2/define_access_keys.sh

export JAVA_HOME=/usr/lib/jvm/java-6-openjdk
export AWS_DEFAULT_REGION=us-east-1
export EC2_URL=https://ec2.us-east-1.amazonaws.com

dir=`dirname $0`
canon_dir=`cd -P -- "$dir" && pwd -P`
export PATH=$canon_dir:$PATH

function load_conf() {
    CONF=$1
    local conffile=`basename "$conf"`
    PREFIX=${conffile%.*}
    echo "Loading config from $conf..." >&2
    . $CONF
}
# echo $PATH
