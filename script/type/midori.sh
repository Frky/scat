#!/bin/bash

curr=`pwd`

cp $1 test/_midori/_build/midori/dump.txt 

cd test/_midori/_build/midori/

pin -t $curr/obj-intel64/type.so -- ./midori 

mv "dump_type.txt" "$curr/log/types/midori/`date +%d%m%y`.log"

rm dump.txt

cd $curr
