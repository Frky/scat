#!/bin/bash

basedir="/media/workdir/phd324/implem/pinalloc"
curr=`pwd`

cd "$basedir"
cp "$1" "test/_emacs/emacs-24.5/src/dump.txt"

cd "test/_emacs/emacs-24.5/src"

pin -t "$basedir/obj-intel64/couple.so" -- ./emacs 2>"$basedir/log/tmp.log"
mv "dump_couple.txt" "$basedir/log/couple/emacs/`date +%d%m%y`.log"

rm dump.txt

cd $curr
