#-*- coding: utf-8 -*-

from datetime import datetime
import re


class TypeAnalysis(object):

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
                addr, fn, args = line[:-1].split(":")
                args = args.split(",")
                if args[0] == '':
                    args = None
                self.log[int(addr)] = (fn, args)


    def parse_data(self):
        raise NotImplemented


    def display(self):
        for addr, (fn, args) in self.log.items():
            line = ""
            if len(args) == 0:
                line += "void "
            else:
                endidx = args[0].find("(")
                if endidx == -1:
                    line += args[0].lower()
                else:
                    line += args[0][0:endidx].lower()
            line += " "
            if fn == "":
                line += hex(addr)
            else:
                line += fn
            line += "("
            if len(args) == 1:
                line += "void"
            if args is None or len(args) == 1:
                line += ");"
                continue
            for i, arg in enumerate(args[1:]):
                endidx = arg.find("(")
                if endidx == -1:
                    line += arg.lower()
                else:
                    line += arg[0:endidx].lower()
                if i != len(args) - 2:
                    line += ", "
            line += ");"
            print line

        print "| Last inference:           {0}".format(self.date)
        print "| Total functions infered:  {0}".format(len(self.log.keys()))

