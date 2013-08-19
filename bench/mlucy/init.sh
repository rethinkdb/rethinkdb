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

export WEB_PORT=8080
export CLIENT_PORT=28015
export CLUSTER_PORT=29015

function update_path() {
    local dir=`dirname $0`
    local canon_dir=`cd -P -- "$dir" && pwd -P`
    export PATH=$canon_dir:$PATH
}
update_path

function load_conf() {
    CONF=$1
    local conffile=`basename "$CONF"`
    PREFIX=${conffile%.*}
    echo "Loading config from $CONF..." >&2
    . $CONF
}

function on_err() {
    echo "ERROR $0:$1" >&2
}
trap 'on_err $LINENO' ERR
# echo $PATH
