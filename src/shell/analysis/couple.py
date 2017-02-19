#-*- coding: utf-8 -*-

from datetime import datetime
import re
from analysis import Analysis
from src.shell.parser.couple import CoupleLogParser

class CoupleAnalysis(Analysis):

    def __init__(self, pgm, logfile, data=None):
        Analysis.__init__(self, pgm, logfile)

        self.data = data
        if data is None:
            self.protos = None

    def parse_log(self):
        self.log = CoupleLogParser(self.logfile)
        return


#        self.log = list()
#        with open(self.logfile, "r") as f:
#            for line in f.readlines():
#                a, b, c, d, e = line[:-1].split(" ")
#                self.log.append((a, c, e))
#

    def display(self):
        for f, g, c in self.log.get():
            print("({0}, {1}) -- {2}".format(f, g, c))
        print()
        self.print_general_info()
