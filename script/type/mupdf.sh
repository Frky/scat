#!/bin/bash

basedir="/media/workdir/phd324/implem/pinalloc"
curr=`pwd`

cd "$basedir"
cp "$1" "test/_mupdf/build/debug/dump.txt"

cd "test/_mupdf/build/debug"

pin -t "$basedir/obj-intel64/type.so" -- ./mupdf-x11 "/media/workdir/Dropbox/inc/Computational-Modeling-of-Narrative-Course-1.pdf" 

mv "dump_type.txt" "$basedir/log/types/mupdf/`date +%d%m%y`.log"
rm dump.txt

cd $curr
