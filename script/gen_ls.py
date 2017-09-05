#!/usr/bin/python

from subprocess import call
from random import choice

cmd = "ls"

args = [
            "-a", # --all
            "-A", # --almost-all
            "  ", #    with -l, print the author of each file
            "-b", # --escape
            "-B", # --ignore-backups
            "-c", #    with  -lt:  sort by, and show, ctime (time of last modification of file status information); with -l: show ctime and sort by name; otherwise: sort by ctime, newest
            "-C", #    list entries by columns
            "-d", # --directory
            "-D", # --dired
            "-f", #    do not sort, enable -aU, disable -ls --color
            "-F", # --classify
            "  ", #    across -x, commas -m, horizontal -x, long -l, single-column -1, verbose -l, vertical -C
            "  ", #    like -l --time-style=full-iso
            "-g", #    like -l, but do not list owner
            "-G", # --no-group
            "-h", # --human-readable
            "  ", #    with -l and/or -s, print human readable sizes (e.g., 1K 234M 2G)
            "-H", # --dereference-command-line
            "  ", #    do not list implied entries matching shell PATTERN (overridden by -a or -A)
            "-i", # --inode
            "-I", # --ignore=PATTERN
            "-k", # --kibibytes
            "-l", #    use a long listing format
            "-L", # --dereference
            "-m", #    fill width with a comma separated list of entries
            "-n", # --numeric-uid-gid
            "  ", #    like -l, but list numeric user and group IDs
            "-N", # --literal
            "-o", #    like -l, but do not list group information
            "-p", # --indicator-style=slash
            "-q", # --hide-control-chars
            "-Q", # --quote-name
            "-r", # --reverse
            "-R", # --recursive
            "-s", # --size
            "-S", #    sort by file size, largest first
            "  ", #    with -l, show time as WORD instead of default modification time: atime or access or use (-u); ctime or status  (-c);  also  use  specified  time  as  sort  key  if
            "  ", #    with  -l,  show  times  using style STYLE: full-iso, long-iso, iso, locale, or +FORMAT; FORMAT is interpreted like in 'date'; if FORMAT is FORMAT1<newline>FORMAT2,
            "  ", #    then FORMAT1 applies to non-recent files and FORMAT2 to recent files; if STYLE is prefixed with 'posix-', STYLE takes effect only outside the POSIX locale
            "-t", #    sort by modification time, newest first
            "-T 7", # --tabsize=COLS
            "-u", #    with -lt: sort by, and show, access time; with -l: show access time and sort by name; otherwise: sort by access time, newest first
            "-U", #    do not sort; list entries in directory order
            "-v", #    natural sort of (version) numbers within text
            "-w 40", # --width=COLS
            "-x", #    list entries by lines instead of by columns
            "-X", #    sort alphabetically by entry extension
            "-Z", # --context
            "-1", #    list one file per line.  Avoid '\n' with -q or -b
        ]

N = 5
TOT = 1000

for j in xrange(1, TOT):
    cli_args = [choice(args) for i in xrange(N)]
    print "ls-{}:".format(j)
    print "    args: \"{}\"".format(" ".join(cli_args))
    print "    bin : \"test/bin/ls/ls-{}\"".format(j)
    print "    data: \"test/data/coreutils.data\""
    print "    deps: \"test/data/coreutils.deps\""
