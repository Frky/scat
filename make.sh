#!/bin/bash


pindir="/usr/bin/pin/source/tools/pinalloc/"
curr=`pwd`
pintooldir="$curr/pintool"

cd "$pindir"

for pin in `ls "$pintooldir"`; do
    pinname=`basename "$pin" .cpp`
    cp -u "$pintooldir"/"$pin" "$pindir"
    make obj-intel64/"$pinname".so
done;

cd "$curr"
