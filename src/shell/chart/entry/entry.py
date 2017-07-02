#-*- coding: utf-8 -*-

class Entry(object):

    def merge(self, entry):
        self._fn_in += entry.fn_in 
        self._fp_in += entry.fp_in 
        self._tot_in += entry.tot_in
        self._fn_out += entry.fn_out
        self._fp_out += entry.fp_out
        self._tot_out += entry.tot_out
        return self

    @property
    def pgm(self):
        return self._pgm

    def set_pgm(self, pgm):
        self._pgm = pgm

    @property
    def fn_in(self):
        return self._fn_in
    
    @property
    def fp_in(self):
        return self._fp_in
        
    @property
    def tot_in(self):
        return self._tot_in

    @property
    def acc_in(self):
        return float(self.tot_in - self.fp_in - self.fn_in)/self.tot_in

    @property
    def fn_out(self):
        return self._fn_out
    
    @property
    def fp_out(self):
        return self._fp_out
        
    @property
    def tot_out(self):
        return self._tot_out

    @property
    def acc_out(self):
        return float(self.tot_out - self.fp_out - self.fn_out)/self.tot_out

    @property
    def time(self):
        return self._time

    @property
    def empty_time(self):
        return self._empty_time

    @property
    def nopin_time(self):
        return self._nopin_time

    @property
    def size(self):
        if self._size > 1024:
            if self._size > 1024*1024:
                return "{} MB".format(self._size / (1024*1024))
            else:
                return "{} KB".format(self._size / (1024))
        else:
            return "{} B".format(self._size)

    def get(self, param):
        if param == "acc_in":
            return self.acc_in
        elif param == "tot_in":
            return self.tot_in
        elif param == "fn_in":
            return self.fn_in
        elif param == "fp_in":
            return self.fp_in
        elif param == "acc_out":
            return self.acc_out
        elif param == "tot_out":
            return self.tot_out
        elif param == "fp_out":
            return self.fp_out
        elif param == "fn_out":
            return self.fn_out
        elif param == "size":
            return self.size
        elif param == "online":
            return self._time
        elif param == "empty":
            return self._empty_time
        elif param == "nopin":
            return self._nopin_time
        else:
            return None

    def _set(self, param, val):
        if param == "acc_in":
            self._acc_in = val
        elif param == "tot_in":
            self._tot_in = val
        elif param == "fn_in":
            self._fn_in = val
        elif param == "fp_in":
            self._fp_in = val
        elif param == "acc_out":
            self._acc_out = val
        elif param == "tot_out":
            self._tot_out = val
        elif param == "fp_out":
            self._fp_out = val
        elif param == "fn_out":
            self._fn_out = val
        elif param == "online":
            self.__time = val
        else:
            pass

