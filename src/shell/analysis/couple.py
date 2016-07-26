#-*- coding: utf-8 -*-

from datetime import datetime
import re
from analysis import Analysis

class CoupleAnalysis(Analysis):

    def __init__(self, pgm, logfile, data=None):
        Analysis.__init__(self, pgm, logfile)


    def parse_log(self):
        self.log = list()
        with open(self.logfile, "r") as f:
            for line in f.readlines():
                a, b, c, d, e = line[:-1].split(" ")
                self.log.append((a, c, e))


    def display(self):
        for f, g, c in self.log:
            print("({0}, {1}) -- {2}".format(f, g, c))
        print()
        self.print_general_info()
