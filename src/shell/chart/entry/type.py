#-*- coding: utf-8 -*-

from .entry import Entry

class TypeEntry(Entry):

    def __init__(self, line, *args, **kwargs):
        l = line[:-1].split(":")
        self._pgm = l[0]
        self._minvals = int(l[1])
        self._maxvals = int(l[2])
        self._addrth = float(l[3])
        self._fn_in, self._fp_in, self._tot_in = map(lambda a: int(a), l[4:7])
        self._fn_out, self._fp_out, self._tot_out = map(lambda a: int(a), l[7:])
        super(TypeEntry, self).__init__(*args, **kwargs)

    @property
    def min_vals(self):
        return int(self.__minvals)
        
    @property
    def max_vals(self):
        return int(self.__maxvals)
        
    @property
    def addr_threshold(self):
        return float(self.__addrth)

    def get(self, param):
        if param == "min_vals":
            return self.min_vals
        if param == "max_vals":
            return self.max_vals
        elif param == "addr_threshold":
            return self.addr_threshold
        else:
            return super(TypeEntry, self).get(param)

