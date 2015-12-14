#-*- coding: utf-8 -*-

from datetime import datetime
import re


class ArityAnalysis(object):

    def __init__(self, pgm, logfile, datafile=None):
        self.pgm = pgm
        self.logfile = logfile
        self.datafile = datafile
        self.date = datetime.fromtimestamp(int(re.search("[0123456789]+", logfile).group()))
        self.log = None
        self.data = None
        self.parse_log()


    def parse_log(self):
        self.log = dict()
        with open(self.logfile, "r") as f:
            for line in f.readlines():
                addr, fn, ar, iar, ret, misc = line[:-1].split(":")
                self.log[int(addr)] = (fn, int(ar), int(ret))


    def parse_data(self):
        raise NotImplemented


    def display(self):
        for addr, fn in self.log.items():
            print hex(addr), fn
        print "| Last inference:           {0}".format(self.date)
        print "| Total functions infered:  {0}".format(len(self.log.keys()))

