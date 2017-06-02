#-*- coding: utf-8 -*-

import os

from src.shell.chart.chart import Chart

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


class TypeEntry(object):

    def __init__(self, line, *args, **kwargs):
        l = line[:-1].split(":")
        self.__pgm = l[0]
        self.__minvals = int(l[1])
        self.__maxvals = int(l[2])
        self.__addrth = float(l[3])
        self.__fn_in, self.__fp_in, self.__tot_in = l[4:7]
        self.__fn_out, self.__fp_out, self.__tot_out = l[7:]
        super(TypeEntry, self).__init__(*args, **kwargs)

    @property
    def pgm(self):
        return self.__pgm

    @property
    def min_vals(self):
        return int(self.__minvals)
        
    @property
    def max_vals(self):
        return int(self.__maxvals)
        
    @property
    def addr_threshold(self):
        return float(self.__addrth)

    @property
    def fn_in(self):
        return int(self.__fn_in)
    
    @property
    def fp_in(self):
        return int(self.__fp_in)
        
    @property
    def tot_in(self):
        return int(self.__tot_in)

    def get(self, param):
        if param == "min_vals":
            return self.min_vals
        if param == "max_vals":
            return self.max_vals
        elif param == "addr_threshold":
            return self.addr_threshold

