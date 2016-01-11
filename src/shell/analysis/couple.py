#-*- coding: utf-8 -*-

from datetime import datetime
import re


class CoupleAnalysis(object):

    def __init__(self, pgm, logfile, data=None):
        self.pgm = pgm
        self.logfile = logfile
        self.date = datetime.fromtimestamp(int(re.search("[0123456789]+", logfile).group()))
        self.data = data
        self.log = None
        self.parse_log()


    def parse_log(self):
        self.log = list()
        with open(self.logfile, "r") as f:
            for line in f.readlines():
                a, b, c, d, e = line[:-1].split(" ")
                self.log.append((a, c, e))


    def print_general_info(self):
        print "Information about inference"
        print "| Last inference:           {0}".format(self.date)
        print "| Total number of couples:  {0}".format(len(self.log))
        l = len(set([f for f, g, c in self.log]))
        r = len(set([g for f, g, c in self.log]))
        print "| Unique left/right-side:   {0}/{1}".format(l, r)


    def display(self):
        for f, g, c in self.log:
            print "({0}, {1}) -- {2}".format(f, g, c)
        print
        self.print_general_info()

