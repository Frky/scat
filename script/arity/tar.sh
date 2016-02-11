#!/bin/bash

basedir=`pwd`

curr=`pwd`

cd "$basedir"
cd "test/_tar/tar-1.28/src"

time pin -t $basedir/obj-intel64/arity.so -- ./tar -xvzof ../../../../tmp/_midori.tar.gz 1>/dev/null
nb_fn_inf=`less dump.txt | wc -l`
mv dump.txt $curr/log/arity/tar/`date +%d%m%y`.log

cd $curr

echo "NB FN INFERED: $nb_fn_inf"
