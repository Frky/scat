# Counting the number of lines in a list of files
# for loop over arguments
# count only those files I am owner of

if [ $# -lt 1 ]
then
    echo "Usage: $0 file ..."
    exit 1
fi

echo "$0 counts the lines of code" 
l=0
n=0
s=0
for f in $*
do
    if [ -O $f ] # checks whether file owner is running the script
    then 
        l=`wc -l $f`
        l=`expr match "$l" "\([0-9]*\)"`
        echo "$f: $l"
        n=$[ $n + 1 ]
        s=$[ $s + $l ]
    else
        continue
    fi
done

echo "$n files in total, with $s lines in total"

