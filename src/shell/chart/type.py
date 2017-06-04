#-*- coding: utf-8 -*-

import os

from .chart import Chart
from .entry.type import TypeEntry

class TypeChart(Chart):

    def __init__(self, *args, **kwargs):
        super(TypeChart, self).__init__(*args, **kwargs)
        self._analysis = "type"
        self.__parse_log()
        self._data = sum(self._data.values(), list())

    def __parse_log(self):
        if not os.path.exists(self._log):
            return
        with open(self._log, "r") as f:
            for line in f.readlines():
                pgm = line.split(":")[0]
                self._data.setdefault(pgm, list())
                entry = TypeEntry(line)
                self._data[pgm].append(entry)
