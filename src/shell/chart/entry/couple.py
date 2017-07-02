#-*- coding: utf-8 -*-

from .entry import Entry

class CoupleEntry(Entry):

    def __init__(self, line, *args, **kwargs):
        l = line[:-1].split(":")
        self._pgm = l[0]
        self._minvals = int(l[1])
        self._maxvals = int(l[2])
        self._rho = float(l[3])
        self._tot = int(l[4])
        self._f, self._g, self._n = map(int, l[5:])
        super(CoupleEntry, self).__init__(*args, **kwargs)

    def merge(self, e):
        self._f += e.f
        self._g += e.g
        self._n += e.n
        self._tot += e.tot
        return self

    def average(self, tot):
        self._f = int(float(self._f)/tot)
        self._g = int(float(self._g)/tot)
        self._n = int(float(self._n)/tot)
        self._tot = int(float(self._tot)/tot)

    @property
    def min_vals(self):
        return int(self._minvals)
        
    @property
    def max_vals(self):
        return int(self._maxvals)
        
    @property
    def rho(self):
        return float(self._rho)

    @property
    def f(self):
        return self._f

    @property
    def g(self):
        return self._g

    @property
    def n(self):
        return self._n

    @property
    def tot(self):
        return self._tot

    def get(self, param):
        if param == "min_vals":
            return self.min_vals
        if param == "max_vals":
            return self.max_vals
        elif param == "rho":
            return self.rho
        elif param == "f":
            return self.f
        elif param == "g":
            return self.g
        elif param == "n":
            return self.n
        elif param == "tot":
            return self.tot
