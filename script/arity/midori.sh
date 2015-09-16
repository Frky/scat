#!/bin/bash

curr=`pwd`

cd test/_midori/_build/midori/

pin -t $curr/obj-intel64/arity.so -- ./midori # 1>/dev/null

mv dump.txt $curr/log/arity/midori/`date +%d%m%y`.log

cd $curr
