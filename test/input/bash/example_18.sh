############################################################################
#
# Usage: loc7.sh [options] file ...
#
# Count the number of lines in a given list of files.
# Uses a for loop over all arguments.
#
# Options:
#  -h     ... help message
#  -d n ... consider only files modified within the last n days
#  -w n ... consider only files modified within the last n weeks
#
# Limitations: 
#  . only one option should be given; a second one overrides
#
############################################################################

help=0
verb=0
weeks=0
# defaults
days=0
m=1
str="days"
getopts "hvd:w:" name
while [ "$name" != "?" ] ; do
    case $name in
        h) help=1;;   
        v) verb=1;;   
        d) days=$OPTARG
            m=$OPTARG
            str="days";;
        w) weeks=$OPTARG
            m=$OPTARG
            str="weeks";;
    esac 
    getopts "hvd:w:" name
done

if [ $help -eq 1 ]
then no_of_lines=`cat $0 | awk 'BEGIN { n = 0; } \
    /^$/ { print n; \
    exit; } \
    { n++; }'`
    echo "`head -$no_of_lines $0`"
    exit 
fi

shift $[ $OPTIND - 1 ]

if [ $# -lt 1 ]
then
    echo "Usage: $0 file ..."
    exit 1
fi

if [ $verb -eq 1 ]
then echo "$0 counts the lines of code" 
fi

l=0
n=0
s=0
for f in $*
do
    x=`stat -c "%y" $f`
    # modification date
    d=`date --date="$x" +%y%m%d`
    # date of $m days/weeks ago
    e=`date --date="$m $str ago" +%y%m%d`
    # now
    z=`date +%y%m%d`
    #echo "Stat: $x; Now: $z; File: $d; $m $str ago: $e"
    # checks whether file is more recent then req
    if [ $d -ge $e -a $d -le $z ] # ToDo: fix year wrap-arounds
    then 
        # be verbose if we found a recent file
        if [ $verb -eq 1 ] 
        then echo "$f: modified (mmdd) $d"
        fi
        # do the line count
        l=`wc -l $f | sed 's/^\([0-9]*\).*$/\1/'`
        echo "$f: $l"
        # increase the counters
        n=$[ $n + 1 ]
        s=$[ $s + $l ]
    else
        # not strictly necessary, because it's the end of the loop
        continue
    fi
done

echo "$n files in total, with $s lines in total"
