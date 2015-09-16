#!/bin/bash

basedir="/media/workdir/phd324/implem/pinalloc"

curr=`pwd`

cd "$basedir"
cd "test/_grep/grep-2.21/src/"

pin -t $curr/obj-intel64/arity.so -- ./grep -R "void" . 

mv dump.txt $curr/log/arity/grep/`date +%d%m%y`.log

cd $curr
