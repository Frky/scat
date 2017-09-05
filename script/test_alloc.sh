pgm=`basename $1`
args=`cat test/coreutils/args/$pgm.txt`

if [ "$args" = "" ]; then
    cmd_line="$1"
else
    cmd_line="$1 $args"
fi

# echo "#$cmd_line#"
# echo "launch arity \"$cmd_line 1> /dev/null\"" | ./scat.py 1,2>/dev/null 
# echo "launch type \"$cmd_line 1> /dev/null\"" | ./scat.py 1,2>/dev/null
# 
# echo "### MALLOC/FREE ###"
# echo "launch memalloc \"$cmd_line 1>/dev/null\"" | ./scat.py 
# echo "memcomb $pgm --lib" | ./scat.py 
# 
# exit

echo "### MALLOC/FAKE_FREE ###"
export LD_PRELOAD="/media/degoerdf/workdir/scat/mem_hook.so" 
echo "launch memalloc \"$cmd_line 1>/dev/null\"" | ./scat.py 
export LD_PRELOAD=""
echo "memcomb $pgm --lib" | ./scat.py 
