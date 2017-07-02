# Counting the number of lines in a list of files
# function version using return code
# WRONG version: the return code is limited to 0-255
#  so this script will run, but print wrong values for
#  files with more than 255 lines

count_lines () {
    local f=$1  
    local m
    m=`wc -l $f`
    l=`expr match "$m" "\([0-9]*\)"`
    return $m
}

if [ $# -lt 1 ]
then
    echo "Usage: $0 file ..."
    exit 1
fi

echo "$0 counts the lines of code" 
l=0
n=0
s=0
while [ "$*" != ""  ]
do
    count_lines $1
    l=$?
    echo "$1: $l"
    n=$[ $n + 1 ]
    s=$[ $s + $l ]
    shift
done

echo "$n files in total, with $s lines in total"
