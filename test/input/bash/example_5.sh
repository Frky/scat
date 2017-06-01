# copy a file, creating a backup if the target file exists
if [ $# -lt 2 ]
then
     echo "Usage: $0 fromfile tofile"
      exit 1
  fi
  if [ -f $2 ]
       then mv $2 $2.bak
       fi
       cp $1 $2
