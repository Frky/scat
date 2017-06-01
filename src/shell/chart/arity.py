#-*- coding: utf-8 -*-

import os

from src.shell.chart.chart import Chart

class ArityChart(Chart):

    def __init__(self, *args, **kwargs):
        super(ArityChart, self).__init__(*args, **kwargs)
        self._analysis = "arity"
        self.__parse_log()
        self._data = sum(self._data.values(), list())

    def __parse_log(self):
        if not os.path.exists(self._log):
            return
        with open(self._log, "r") as f:
            for line in f.readlines():
                pgm = line.split(":")[0]
                self._data.setdefault(pgm, list())
                entry = ArityEntry(line)
                self._data[pgm].append(entry)


class ArityEntry(object):

    def __init__(self, line, *args, **kwargs):
        l = line[:-1].split(":")
        self.__pgm = l[0]
        self.__mincalls = int(l[1])
        self.__paramth, self.__retth = map(lambda a: float(a), l[2:4])
        self.__fn_in, self.__fp_in, self.__tot_in = l[4:7]
        self.__fn_out, self.__fp_out, self.__tot_out = l[7:]
        super(ArityEntry, self).__init__(*args, **kwargs)

    @property
    def pgm(self):
        return self.__pgm

    @property
    def min_calls(self):
        return int(self.__mincalls)
        
    @property
    def param_threshold(self):
        return float(self.__paramth)

    @property
    def ret_threshold(self):
        return float(self.__retth)

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
        if param == "min_calls":
            return self.min_calls
        elif param == "ret_threshold":
            return self.ret_threshold
        elif param == "param_threshold":
            return self.param_threshold

