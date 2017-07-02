#-*- coding: utf-8 -*-

import os

from .chart import Chart
from .entry.arity import ArityEntry

class ArityChart(Chart):

    def __init__(self, logfile, testconf, *args, **kwargs):
        super(ArityChart, self).__init__(logfile)
        self._analysis = "arity"
        self.__parse_log(testconf)
        self._data = sum(self._data.values(), list())

    def __parse_log(self, testconf):
        print self._log
        if not os.path.exists(self._log):
            return
        with open(self._log, "r") as f:
            for line in f.readlines():
                pgm = line.split(":")[0]
                if pgm in testconf.keys():
                    size = os.path.getsize(testconf[pgm]["bin"])
                else:
                    size = 0
                self._data.setdefault(pgm, list())
                entry = ArityEntry(line, size)
                self._data[pgm].append(entry)

