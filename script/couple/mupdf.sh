#!/bin/bash

basedir="/media/workdir/phd324/implem/pinalloc"
curr=`pwd`

cd "$basedir"
cp "$1" "test/_mupdf/build/debug/dump.txt"

cd "test/_mupdf/build/debug"

pin -t "$basedir/obj-intel64/couple.so" -- ./mupdf-x11 "/media/workdir/Dropbox/inc/Computational-Modeling-of-Narrative-Course-1.pdf"

mv "dump_couple.txt" "$basedir/log/couple/mupdf/`date +%d%m%y`.log"

rm dump.txt

cd $curr
