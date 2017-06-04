#-*- coding: utf-8 -*-

import os

from src.shell.chart.chart import Chart
from .entry.arity import ArityEntry
from .entry.type import TypeEntry

class AccuracyChart(Chart):

    def __init__(self, analysis, *args, **kwargs):
        super(AccuracyChart, self).__init__(*args, **kwargs)
        self._analysis = analysis
        self.__parse_log()
        self._data = sum(self._data.values(), list())

    def __parse_log(self):
        if not os.path.exists(self._log):
            return
        with open(self._log, "r") as f:
            for line in f.readlines():
                pgm = line.split(":")[0]
                self._data.setdefault(pgm, list())
                if self._analysis == "arity":
                    entry = ArityEntry(line)
                else:
                    entry = TypeEntry(line)
                self._data[pgm].append(entry)

