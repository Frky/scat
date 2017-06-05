# Counting the number of lines in a list of files
# function version

# function storing list of all files in variable files
get_files () {
    files="`find $1 -name *.[ch]`"
}

# function counting the number of lines in a file
count_lines () {
    f=$1  # 1st argument is filename
    l=`wc -l $f`
    l=`expr match "$l" "\([0-9]*\)"`
}

# the script should be called without arguments
if [ $# -ne 1 ]
then
    echo "Usage: $0 src"
    exit 1
fi

# split by newline
IFS=$'\012'

echo "$0 counts the lines of code" 
# don't forget to initialise!
l=0
n=0
s=0
# call a function to get a list of files
get_files $1
# iterate over this list
for f in $files
do
    # call a function to count the lines
    count_lines $f
    echo "$f: $l"loc
    # store filename in an array
    file[$n]=$f
    # store number of lines in an array
    lines[$n]=$l
    # increase counter
    n=$[ $n + 1 ]
    # increase sum of all lines
    s=$[ $s + $l ]
done

echo "$n files in total, with $s lines in total"
i=5
echo "The $i-th file was ${file[$i]} with ${lines[$i]} lines"

