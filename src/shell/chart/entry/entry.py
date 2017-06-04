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

    def get(self, param):
        if param == "acc_in":
            return self.acc_in
        elif param == "tot_in":
            return self.tot_in
        elif param == "fp_in":
            return self.fn_in
        elif param == "fn_in":
            return self.fp_in
        elif param == "acc_out":
            return self.acc_out
        elif param == "tot_out":
            return self.tot_out
        elif param == "fp_out":
            return self.fp_out
        elif param == "fn_out":
            return self.fn_out
        else:
            return None
