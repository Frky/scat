#!/bin/bash

echo "launch arity \"$* 1> /dev/null\"" | ./scat.py 
echo "launch type \"$* 1> /dev/null\"" | ./scat.py 

echo "launch memalloc \"$* 1>/dev/null\"" | ./scat.py 
echo "memcomb `basename $1`" | ./scat.py 
echo "memcomb `basename $1` --lib" | ./scat.py 

