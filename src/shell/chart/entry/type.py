#-*- coding: utf-8 -*-

from .entry import Entry

class TypeEntry(Entry):

    def __init__(self, line, size=0, *args, **kwargs):
        l = line[:-1].split(":")
        self._pgm = l[0]
        self._minvals = int(l[1])
        self._maxvals = int(l[2])
        self._addrth = float(l[3])
        self._fp_in, self._fn_in, self._tot_in = map(lambda a: int(a), l[4:7])
        self._fp_out, self._fn_out, self._tot_out = map(lambda a: int(a), l[7:10])
        self._time = float(l[10])
        self._empty_time = float(l[11])
        self._nopin_time = float(l[12])
        self._size = size
        super(TypeEntry, self).__init__(*args, **kwargs)

    @property
    def min_vals(self):
        return int(self._minvals)
        
    @property
    def max_vals(self):
        return int(self._maxvals)
        
    @property
    def addr_threshold(self):
        return float(self._addrth)

    def get(self, param):
        if param == "min_vals":
            return self.min_vals
        if param == "max_vals":
            return self.max_vals
        elif param == "addr_threshold":
            return self.addr_threshold
        else:
            return super(TypeEntry, self).get(param)

