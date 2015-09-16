#!/bin/bash

basedir="/media/workdir/phd324/implem/pinalloc"
curr=`pwd`

cd "$basedir"
cp "$1" "test/_midori/_build/midori/dump.txt"

cd "test/_midori/_build/midori/"

pin -t "$basedir/obj-intel64/couple.so" -- ./midori 

mv "dump_couple.txt" "$basedir/log/couple/midori/`date +%d%m%y`.log"

rm dump.txt

cd $curr
