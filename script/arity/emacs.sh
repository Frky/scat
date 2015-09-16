#!/bin/bash

basedir="/media/workdir/phd324/implem/pinalloc"

curr=`pwd`

cd "$basedir"
cd "test/_emacs/emacs-24.5/src/"

time pin -t $curr/obj-intel64/arity.so -- ./emacs 
nb_fn_inf=`less dump.txt | wc -l`

mv dump.txt $curr/log/arity/emacs/`date +%d%m%y`.log

cd $curr

echo "NB FN INFERED: $nb_fn_inf"
