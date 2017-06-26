#-*- coding: utf-8 -*-

import sys

class Std(object):

    verbose = 1

    def stdout(self, m, crlf=True):
        if Std.verbose > 0:
            sys.stdout.write("[*] " + msg)
            if crlf:
                sys.stdout.write("\n")

    def stderr(self, msg):
        if Std.verbose >= 0:
            sys.stderr.write("*** " + msg + "\n")

