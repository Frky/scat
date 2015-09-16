#!/bin/bash

basedir="/media/workdir/phd324/implem/pinalloc"

curr=`pwd`

cd "$basedir"
cd "test/_mupdf/build/debug/"

time pin -t $curr/obj-intel64/arity.so -- ./mupdf-x11 /media/workdir/Dropbox/inc/Computational-Modeling-of-Narrative-Course-1.pdf
nb_fn_inf=`less dump.txt | wc -l`
mv dump.txt $curr/log/arity/mupdf/`date +%d%m%y`.log

cd $curr

echo "NB FN INFERED: $nb_fn_inf"
