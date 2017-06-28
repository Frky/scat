#-*- coding: utf-8 -*-

from .entry import Entry

class ArityEntry(Entry):

    def __init__(self, line="", size=0, *args, **kwargs):
        if line != "":
            l = line[:-1].split(":")
            self._pgm = l[0]
            self._mincalls = int(l[1])
            self._paramth, self._retth = map(lambda a: float(a), l[2:4])
            self._fn_in, self._fp_in, self._tot_in = map(lambda a: int(a), l[4:7])
            self._fn_out, self._fp_out, self._tot_out = map(lambda a: int(a), l[7:10])
            self._time = float(l[10])
            self._empty_time = float(l[11])
            self._nopin_time = float(l[12])
            self._size = size
        for p, v in kwargs.items():
            self._set(p, v)
        super(ArityEntry, self).__init__()

    @property
    def min_calls(self):
        return int(self._mincalls)
        
    @property
    def param_threshold(self):
        return float(self._paramth)

    @property
    def ret_threshold(self):
        return float(self._retth)

    def get(self, param):
        if param == "min_calls":
            return self.min_calls
        elif param == "ret_threshold":
            return self.ret_threshold
        elif param == "param_threshold":
            return self.param_threshold
        else:
            return super(ArityEntry, self).get(param)

    def _set(self, param, val):
        if param == "min_calls":
            self._min_calls = val
        elif param == "ret_threshold":
            self._ret_threshold = val
        elif param == "param_threshold":
            self._param_threshold = val
        else:
            super(ArityEntry, self)._set(param, val)

