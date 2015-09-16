#!/bin/bash

basedir="/media/workdir/phd324/implem/pinalloc"
curr=`pwd`

cd "$basedir"
cp "$1" "test/_grep/grep-2.21/src/dump.txt"

cd "test/_grep/grep-2.21/src"

pin -t "$basedir/obj-intel64/type.so" -- ./grep -R "void" . 1>/dev/null
cp dump_type.txt "$basedir/log/types/grep/`date +%d%m%y`.log"

rm dump.txt

cd $curr
